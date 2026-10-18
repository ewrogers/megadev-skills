#include "lab.h"
#include "shared.h"
#include "save.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_font[];
#define CELLS 1120
#define DIRTY_CAP 96
/* Only the callback writes VDP ports during normal display. */
typedef struct {u16 offset,word;} Cell;
typedef struct {Cell cells[DIRTY_CAP];u16 count;u16 sat[8][4],sprites;} Frame;
_Static_assert(sizeof(Frame)==452,"Publisher fixture layout changed");
static u16 desired[CELLS],displayed[CELLS],scan,stage,hz,field,help,request_op;
static SaveGame game={1,10,0};
static Frame frames[2];
static volatile u16 published,submitted,video_fault,input_hits;
extern void system_install(void);
#ifdef SAVE_AUTOTEST
static u16 tour_ok;
#endif
void lab_line(char const *s,u16 x,u16 y) {
    char line[41];u16 n=0;
    if(x>=40 || y>=28)return;
    while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
    lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128+x*2)|VRAM_W);
}
void lab_hex(u16 value,u16 x,u16 y) {
    static char const h[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=h[(value>>(12-i*4))&15];
    s[4]=0;lab_line(s,x,y);
}
void lab_screen(char const *title){lab_line(title,1,0);}
void vblank_user(void) {
    main_ticks++;input_hits|=bios_joy1_hit;
    u16 index=published;if(!index)return;
    Frame const *f=&frames[index-1];
    if(f->count>DIRTY_CAP || f->sprites>8){video_fault=1;published=0;return;}
    for(u16 i=0;i<f->count;i++)if(f->cells[i].offset>=CELLS){video_fault=1;published=0;return;}
    for(u16 i=0;i<f->count;i++) {
        u16 cell=f->cells[i].offset;
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(cell/40u)*128u+(cell%40u)*2u)|VRAM_W;
        vdp_data_16=f->cells[i].word;displayed[cell]=f->cells[i].word;
    }
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    u16 sprites=f->sprites?f->sprites:1;
    for(u16 i=0;i<sprites;i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sat[i][w];
    submitted++;published=0;
}
static void text(u16 x,u16 y,u16 color,char const *s) {
    while(*s && x<40 && y<28){desired[y*40+x]=(u16)(0x8000|(color<<13)|(u8)*s);x++;s++;}
}
static void hex(u16 x,u16 y,u16 color,u16 value) {
    static char const h[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=h[(value>>(12-i*4))&15];
    s[4]=0;text(x,y,color,s);
}
static void draw(void) {
    static char const * const names[]={"READY","SAVED AND VERIFIED","LOADED","OLDER RECORD RECOVERED","NO SAVE YET","MEDIA NOT READY","BIOS IO FAILED / CHECK SPACE","NEWER FORMAT - LEFT UNCHANGED","INVALID DATA / NEED TWO SAVES","UNEXPECTED FILE SIZE OR MODE","READBACK VERIFICATION FAILED","SEQUENCE FULL - LEFT UNCHANGED","CONFLICTING RECORDS"};
    for(u16 i=0;i<CELLS;i++)desired[i]=0x8020;
    text(1,0,0,"SAVE LAB");text(23,0,1,save_device?"BACKUP CART":"INTERNAL BACKUP");
    for(u16 i=0;i<40;i++)text(i,1,0,"-");
    text(1,3,0,"YOUR JOURNEY");text(1,5,field==0?1:0,field==0?"> LEVEL":"  LEVEL");hex(17,5,1,game.level);
    text(1,7,field==1?1:0,field==1?"> COINS":"  COINS");hex(17,7,1,game.coins);
    text(1,9,field==2?1:0,field==2?"> FLAGS":"  FLAGS");hex(17,9,1,game.flags);
    text(1,11,field==3?1:0,field==3?"> DEVICE":"  DEVICE");text(17,11,1,save_device?"BACKUP CART":"INTERNAL");
    text(1,13,0,"STATUS");text(9,13,save_result==SV_RECOVERED?2:1,save_pending?save_fault?"WAITING - SLOT STAYS OWNED":"WORKING":names[save_result<=SV_CONFLICT?save_result:SV_IO]);
    text(1,15,0,"MEDIA");text(9,15,2,save_media==3?"SEGA FORMATTED":save_media==1?"UNFORMATTED":save_media==2?"OTHER FORMAT":save_media==4?"UNSUPPORTED CART":"NOT READY");
    text(1,16,0,"SEQUENCE");hex(12,16,1,save_sequence);
    if(help) {
        text(1,18,0,"SAVE TWO DIFFERENT VERSIONS FIRST");
        text(1,19,0,"A DAMAGE NEWEST AND RECOVER OLDER");
        text(1,20,0,"B BACK - ONLY LAB FILES ARE TOUCHED");
        text(1,21,0,"LOAD FALLS BACK TO THE OLDER RECORD");
        text(1,22,2,"UNFORMATTED? USE BIOS MEMORY MANAGER");
    } else {
        text(1,18,0,"UP/DOWN FIELD   LEFT/RIGHT CHANGE");
        text(1,19,0,"A SAVE   B LOAD   C RESET VALUES");
        text(1,21,0,"START HELP / CORRUPTION DEMO");
        text(1,22,2,"NO AUTOMATIC FORMAT OR DELETE");
    }
    text(1,24,0,"VALUES AND COUNTERS ARE HEXADECIMAL");
    text(1,26,0,"MAIN");hex(6,26,1,main_ticks);text(15,26,0,"SUB");hex(19,26,1,*ga_reg_comstat3);text(34,26,0,hz==50?"PAL":"NTSC");
#ifdef SAVE_AUTOTEST
    text(1,27,1,tour_ok?"AUTO PASS - READ ONLY MEDIA PROBE":"AUTOMATIC READ ONLY MEDIA PROBE");
#endif
}
static void publish(void) {
    Frame *f=&frames[stage];f->count=0;f->sprites=0;
    u16 checked=0;
    while(checked<CELLS && f->count<DIRTY_CAP) {
        u16 cell=scan;scan++;if(scan==CELLS)scan=0;checked++;
        if(desired[cell]!=displayed[cell]){f->cells[f->count].offset=cell;f->cells[f->count].word=desired[cell];f->count++;}
    }
    for(u16 i=0;i<4;i++)f->sat[0][i]=0;
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize_video(void) {
    lab_display_off();lab_clear();
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    u16 const colors[4]={0x0EEE,0x06E6,0x06CE,0x068E};
    for(u16 p=0;p<4;p++)for(u16 i=0;i<16;i++)bios_palette[p*16+i]=i==1?(s16)colors[p]:(i==2?0x0220:0);
    bios_vdp_update_flags=0;vdp_ctrl_32=CRAM_W;
    for(u16 i=0;i<64;i++)vdp_data_16=(u16)bios_palette[i];
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){displayed[y*40+x]=0x8020;vdp_data_16=0x8020;}
    }
}
static void save_issue(u16 op) {
    request_op=op;
    if(save_request(op,&game) && save_device && (op==3 || op==4) && (save_result==SV_LOADED || save_result==SV_RECOVERED))game=save_reply;
}
void save_input(u16 hit) {
        if(hit&PAD_START)help^=1;
        if(!save_pending && help) {
            if(hit&PAD_B)help=0;
            else if(hit&PAD_A){save_issue(4);}
        } else if(!save_pending) {
            if(hit&PAD_DOWN){field++;if(field==4)field=0;}
            if(hit&PAD_UP)field=field?field-1:3;
            if((hit&(PAD_LEFT|PAD_RIGHT)) && field==3){save_device^=1;save_issue(1);return;}
            if(hit&(PAD_LEFT|PAD_RIGHT)) {
                u16 *v=field==0?&game.level:field==1?&game.coins:&game.flags;
                u16 low=field==0?1:0,high=field==0?9:field==1?999:3;
                if(hit&PAD_RIGHT)*v=*v==high?low:(u16)(*v+1);else *v=*v==low?high:(u16)(*v-1);
            }
            if(hit&PAD_C)game=(SaveGame){1,0,0};
            request_op=0;
            if(hit&PAD_A)request_op=2;
            else if(hit&PAD_B)request_op=3;
            if(request_op)save_issue(request_op);
        }
}
void main(void) {
    disable_interrupts();system_install();
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    initialize_video();draw();
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){u16 cell=y*40+x;displayed[cell]=desired[cell];vdp_data_16=desired[cell];}
    }
    lab_display_on();enable_interrupts();
    u16 previous_tick=main_ticks;save_request(1,&game);
    for(;;) {
        lab_wait();u16 delta=(u16)(main_ticks-previous_tick);previous_tick=main_ticks;
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
        u16 pending=save_pending;save_main_tick(delta>=hz?300:(u16)(delta*(hz==50?6u:5u)));
        if(pending==1 && save_pending!=1 && save_pending!=3 && (request_op==3 || request_op==4) && (save_result==SV_LOADED || save_result==SV_RECOVERED))game=save_reply;
#ifdef SAVE_AUTOTEST
        hit=0;if(!save_pending && !save_fault && !video_fault)tour_ok=1;
#endif
        save_input(hit);
        draw();publish();
    }
}

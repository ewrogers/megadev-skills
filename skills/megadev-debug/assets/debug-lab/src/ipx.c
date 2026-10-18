#include "lab.h"
#include "shared.h"
#include "link.h"
#include "trace.h"
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
static u16 desired[CELLS],displayed[CELLS],stage,hz,profile,rover_x=20,rover_y=23;
static u16 dirty_cells[CELLS],dirty_head,dirty_tail,dirty_count;
static unsigned char dirty_mark[CELLS];
static u16 page;
static Frame frames[2];
static volatile u16 published,submitted,video_fault,input_hits;
extern void system_install(void);
#ifdef SYSTEM_AUTOTEST
static u16 tour_step,tour_due=60,tour_finished,tour_ok;
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
    u16 cell=(u16)(y*40u+x);
    while(*s && x<40 && y<28) {
        u16 value=(u16)(0x8000|(color<<13)|(u8)*s);
        if(desired[cell]!=value) {
            desired[cell]=value;
            if(!dirty_mark[cell]) {
                if(dirty_count==CELLS){video_fault=1;return;}
                dirty_mark[cell]=1;dirty_cells[dirty_tail]=cell;
                dirty_tail++;if(dirty_tail==CELLS)dirty_tail=0;dirty_count++;
            }
        }
        cell++;x++;s++;
    }
}
static void hex(u16 x,u16 y,u16 color,u16 value) {
    static char const h[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=h[(value>>(12-i*4))&15];
    s[4]=0;text(x,y,color,s);
}
static char const *phase_name(u16 phase) {
    static char const * const names[]={"READY","GIVING BUFFER","WORKING","DRAINING LATE JOB","RETIRING REPLY","OWNERSHIP STUCK"};
    return names[phase<6?phase:5];
}
static char const *event_name(u16 e) {
    static char const * const names[]={"NONE","START","PHASE","REPLY","TIMEOUT","REJECT","GUARD","CANCEL","OWNER"};
    return names[e<=8?e:0];
}
static void draw(void) {
    /* Clear only on page switches; stable cells never enter the transfer queue. */
    static u16 old_page=99,last_head=99,last_count=99;
    u16 redraw=old_page!=page;
    if(old_page!=page){for(u16 y=0;y<28;y++)text(0,y,0,"                                        ");old_page=page;}
    text(1,0,0,"DEBUG LAB / SIGNAL ROOM");text(1,1,0,"--------------------------------------");
    text(1,3,0,"CASE");text(7,3,2,profile==0?"HEALTHY     ":profile==1?"BAD COMMAND ":profile==2?"INVALID SIZE":"BUFFER GUARD");
    text(1,5,0,"STATE");text(8,5,1,"                              ");text(8,5,1,phase_name(client.phase));
    if(!page) {
        text(1,7,0,"FIRST FAULT");text(15,7,main_trace.frozen?3:1,main_trace.frozen?event_name(main_trace.first.event):"NONE   ");
        text(1,9,0,"TOKEN");hex(8,9,1,main_trace.frozen?main_trace.first.token:client.token);
        text(19,9,0,"DETAIL");hex(29,9,2,main_trace.frozen?main_trace.first.detail:client.result);
        text(1,11,0,"ACCEPTED");hex(11,11,1,passed);text(21,11,0,"REJECTED");hex(33,11,2,rejected);
        text(1,12,0,"DISCARDED");hex(11,12,2,drained);text(21,12,0,"BAD DATA");hex(33,12,3,verify_failed);
        text(1,14,0,main_trace.frozen?"TRACE FROZEN / WORK CAN STILL DRAIN":"RECORDING MAIN TRANSITIONS         ");
        text(1,16,0,"A RUN CASE   B DELAYED HEALTHY JOB");
        text(1,17,0,"LEFT/RIGHT CASE   C CLEAR WHEN IDLE");
        text(1,19,0,"START TRACE PAGE / UP-DOWN LIGHT");
        text(1,21,0,"SUB TRACE IS PRIVATE / ELF SYMBOL");
        text(1,22,0,"CLEAR MAIN TRACE BEFORE EACH TEST");
        for(u16 y=23;y<26;y++)text(0,y,0,"                                        ");
        text(rover_x,rover_y,1,"+");
    } else {
        text(1,7,0,"TICK EVENT   TOKEN PHASE OWNER DETAIL");
        u16 count=main_trace.count;u16 first=count>8?count-8:0;
        if(redraw || last_head!=main_trace.head || last_count!=count) {
        last_head=main_trace.head;last_count=count;
        for(u16 i=0;i<8;i++) {
            u16 y=9+i;text(0,y,0,"                                        ");DbgEvent e;
            if(dbg_get(&main_trace,first+i,&e)) {
                hex(1,y,0,e.tick);text(6,y,1,event_name(e.event));hex(14,y,0,e.token);
                hex(20,y,0,e.phase);hex(26,y,2,e.owner);hex(32,y,0,e.detail);
            }
        }
        }
        text(1,19,0,"ORDER: LOCAL MAIN TICKS / HEX VALUES");
        text(1,21,0,"OVERWRITTEN");hex(14,21,2,main_trace.overwritten);
        text(1,23,0,"START SUMMARY / C CLEAR WHEN IDLE");
    }
    text(1,26,0,"MAIN");hex(6,26,1,main_ticks);text(15,26,0,"SUB");hex(19,26,1,*ga_reg_comstat3);text(34,26,0,hz==50?"PAL":"NTSC");
#ifdef SYSTEM_AUTOTEST
    text(1,27,tour_ok?1:2,tour_finished?(tour_ok?"AUTO PASS - MAIN STILL RUNNING":"AUTO FAILED"):"AUTOMATIC DIAGNOSTIC TOUR");
#endif
}
static void publish(void) {
    Frame *f=&frames[stage];f->count=0;f->sprites=0;
    while(dirty_count && f->count<DIRTY_CAP) {
        u16 cell=dirty_cells[dirty_head];dirty_mark[cell]=0;
        dirty_head++;if(dirty_head==CELLS)dirty_head=0;dirty_count--;
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
        for(u16 x=0;x<40;x++){desired[y*40+x]=displayed[y*40+x]=0x8020;vdp_data_16=0x8020;}
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
    for(u16 i=0;i<CELLS;i++)dirty_mark[i]=0;
    dirty_head=dirty_tail=dirty_count=0;
    lab_display_on();enable_interrupts();
    u16 previous_tick=main_ticks;
    for(;;) {
        lab_wait();
        u16 delta=(u16)(main_ticks-previous_tick);previous_tick=main_ticks;
        u16 held=bios_joy1_hold;
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
        if(hit&PAD_LEFT){profile=(profile+3u)&3u;}
        if(hit&PAD_RIGHT){profile=(profile+1u)&3u;}
        if((held&PAD_UP) && rover_y>23)rover_y--;
        if((held&PAD_DOWN) && rover_y<25)rover_y++;
#ifdef SYSTEM_AUTOTEST
        hit=0;rover_x=(u16)(4+((main_ticks>>2)&31u));
        if(!tour_finished && tour_step<5 && client.phase==SY_IDLE && (u16)(main_ticks-tour_due)<0x8000) {
            profile=tour_step==1?1:0;
            if(sy_main_start(profile,tour_step==3)) {tour_step++;tour_due=(u16)(main_ticks+30);}
        }

        if(tour_step==5 && client.phase==SY_IDLE) {
            tour_finished=1;tour_ok=passed==2 && rejected==1 && drained==2 && !verify_failed && !video_fault;
        }
#endif
        if(hit&PAD_START)page^=1;
        if((hit&PAD_C) && client.phase==SY_IDLE)dbg_clear(&main_trace);
        if(hit&(PAD_A|PAD_B))sy_main_start((hit&PAD_B)?0:profile,(hit&PAD_B)!=0);
        sy_main_tick(delta>=hz?300:(u16)(delta*(hz==50?6u:5u)));
#ifdef SYSTEM_AUTOTEST
        if(tour_step==3 && client.phase==SY_WAIT)sy_main_cancel();
#endif
        draw();publish();
    }
}

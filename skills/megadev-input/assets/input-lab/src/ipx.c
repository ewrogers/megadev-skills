#include "lab.h"
#include "shared.h"
#include "input.h"
#include "platform.h"
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
static u16 desired[CELLS],displayed[CELLS],scan,stage,hz,demo,selected,page,tick,remainder,auto_pass;
static Frame frames[2];
static InSample physical[8],simulated[8];
static InSlot live[8],shown[8];
static volatile u16 published,submitted,video_fault;
static char const * const type_names[]={"NONE","PAD3","PAD6","MOUSE","OTHER"};
static char const * const errors[]={"OK","EMPTY","TIMEOUT","BAD DATA","OVERFLOW","BUS BUSY"};
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
    main_ticks++;
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
static void signed_value(u16 x,u16 y,s16 value) {
    char s[5];s[0]=value<0?'-':'+';u16 v=value<0?(u16)-value:(u16)value;
    s[1]='0';while(v>=100){s[1]++;v-=100;}s[2]='0';while(v>=10){s[2]++;v-=10;}s[3]=(char)('0'+v);s[4]=0;text(x,y,1,s);
}
static void samples(u16 t) {
    /* Scripted Team Player packets, not emulated hardware presence. */
    for(u16 port=0;port<2;port++) {
        u16 a=(u16)(t+port*40u)%180u,b=(u16)(t+port*40u+20u)%180u,c=(u16)(t+port*40u+40u)%180u;
        static u8 const seed[17]={0,0,0,1,2,15,15,15,15,15,15,0,0,0,2,0,0};
        u8 n[17];for(u16 i=0;i<17;i++)n[i]=seed[i];
        if(a>=30 && a<35)n[7]&=7;
        if(b>=30 && b<35)n[9]&=7;
        if(c>=30 && c<35)n[12]|=8;
        if((t/20)&1)n[6]&=14;
        if((t/15)&1)n[10]&=11;
        if((t/60)&1){n[11]=1;n[13]=15;n[14]=14;}
        n[15]=0;n[16]=((t/30)&1)?1:0;
        in_team(n,17,simulated+port*4);
    }
}
static void draw(void) {
    for(u16 i=0;i<CELLS;i++)desired[i]=0x8020;
    text(1,0,0,"INPUT LAB");text(27,0,demo?2:1,demo?"SCRIPTED DEMO":"LIVE DEVICES");
    text(1,1,0,"A VIEW  B SLOT  C DEMO  START JOIN");
    if(auto_pass && demo)text(1,2,1,"AUTO PASS - DEMO CONTINUES");
    else if(demo)text(1,2,2,"TWO SCRIPTED TEAM PLAYER PACKETS");
    else text(1,2,0,INPUT_EA_MODE?"EA 4 WAY PLAY / OWNS BOTH PORTS":"AUTO / PADS MOUSE TEAM PLAYER");
    for(u16 y=4;y<20;y++)text(19,y,0,"|");
    for(u16 i=0;i<40;i++)text(i,3,0,"-");
    for(u16 i=0;i<8;i++) {
        InSlot const *s=&shown[i];InSample const *v=demo?&simulated[i]:&physical[i];
        u16 x=(i<4?0:20),y=(u16)(4+(i&3)*4);
        char name[4]={(char)('1'+(i>>2)),(char)('A'+(i&3)),0,0};
        u16 color=v->status==IN_OK?1:(v->status==IN_ABSENT?0:3);
        text(x,y,selected==i?2:0,selected==i?">":" ");text(x+1,y,color,name);
        text(x+4,y,color,type_names[v->kind<5?v->kind:4]);
        if(s->player){text(x+11,y,2,"P");char p[2]={(char)('0'+s->player),0};text(x+12,y,2,p);}
        text(x,y+1,0,"H ");hex(x+2,y+1,color,s->held);text(x+7,y+1,0,"+ ");hex(x+9,y+1,1,s->pressed);
        text(x,y+2,0,"- ");hex(x+2,y+2,2,s->released);text(x+7,y+2,0,"R ");hex(x+9,y+2,1,s->repeat);
        if(v->kind==IN_MOUSE && v->status==IN_OK){text(x,y+3,0,"DX");signed_value(x+2,y+3,v->dx);text(x+7,y+3,0,"DY");signed_value(x+9,y+3,v->dy);}
        else text(x,y+3,color,errors[v->status<6?v->status:3]);
    }
    InSlot const *s=&shown[selected];
    if(!page) {
        text(1,20,0,"BUTTONS FOR SELECTED SOCKET");
        u16 const buttons[12]={IN_UP,IN_DOWN,IN_LEFT,IN_RIGHT,IN_A,IN_B,IN_C,IN_START,IN_X,IN_Y,IN_Z,IN_MODE};
        char const * const labels[12]={"U","D","L","R","A","B","C","S","X","Y","Z","M"};
        for(u16 i=0;i<12;i++)text((u16)(2+i*3),21,(s->held&buttons[i])?1:0,labels[i]);
        text(1,22,0,"MOUSE CURSORS / MOVE AND CLICK");
    } else {
        text(1,20,0,"H HELD  + PRESS  - RELEASE  R REPEAT");
        text(1,21,0,"1A-1D PORT 1 / 2A-2D PORT 2");
        text(1,22,0,"FIRST SAMPLE REBASES / NO FALSE PRESS");
    }
    text(1,26,0,"SUB");hex(5,26,1,*ga_reg_comstat3);
    text(12,26,0,"POLL ONCE PER FRAME");text(35,26,0,hz==50?"PAL":"NTSC");
    text(1,27,2,"MULTITAPS NEED REAL HARDWARE CHECKS");
}
static void publish(void) {
    Frame *f=&frames[stage];f->count=0;f->sprites=0;
    u16 checked=0;
    while(checked<CELLS && f->count<DIRTY_CAP) {
        u16 cell=scan;scan++;if(scan==CELLS)scan=0;checked++;
        if(desired[cell]!=displayed[cell]){f->cells[f->count].offset=cell;f->cells[f->count].word=desired[cell];f->count++;}
    }
    for(u16 i=0;i<8;i++)if(shown[i].kind==IN_MOUSE && shown[i].initialized) {
        u16 n=f->sprites;
        if(n)f->sat[n-1][1]=n;
        f->sat[n][0]=(u16)(shown[i].y+128);f->sat[n][1]=0;
        f->sat[n][2]=(u16)(0x8100|((i&3)<<13));f->sat[n][3]=(u16)(shown[i].x+128);f->sprites++;
    }
    if(!f->sprites)for(u16 i=0;i<4;i++)f->sat[0][i]=0;
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize_video(void) {
    lab_display_off();lab_clear();
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    u16 const colors[4]={0x0EEE,0x06E6,0x06CE,0x068E};
    for(u16 p=0;p<4;p++)for(u16 i=0;i<16;i++)bios_palette[p*16+i]=i==1?(s16)colors[p]:(i==2?0x0220:0);
    bios_vdp_update_flags=0;vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++)vdp_data_16=(u16)bios_palette[i];
    /* White crosshair; transparent outside. Shared indices use the UI palettes. */
    vdp_ctrl_32=to_vdp_addr(0x100*32)|VRAM_W;
    for(u16 y=0;y<8;y++) {
        vdp_data_16=y==3?0x1111:0x0001;vdp_data_16=y==3?0x1111:0;
    }
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){displayed[y*40+x]=0x8020;vdp_data_16=0x8020;}
    }
}
void main(void) {
    disable_interrupts();input_install();
    for(u16 i=0;i<8;i++){in_reset(&live[i]);in_reset(&shown[i]);in_clear(&physical[i],IN_ABSENT);in_clear(&simulated[i],IN_ABSENT);}
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
#ifdef INPUT_AUTOTEST
    demo=1;
#endif
    initialize_video();draw();
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){u16 cell=y*40+x;displayed[cell]=desired[cell];vdp_data_16=desired[cell];}
    }
    enable_interrupts();lab_wait(); /* One full interval after the old BIOS poll. */
    lab_display_on();
    for(;;) {
        input_poll(physical);
        for(u16 i=0;i<8;i++){in_update(&live[i],&physical[i],hz);in_join(live,i);}
        u16 hit=0;for(u16 i=0;i<8;i++)hit|=live[i].pressed;
        if(hit&IN_A)page^=1;
        if(hit&IN_B)selected=(selected+1)&7;
        if(hit&IN_C){demo^=1;tick=0;remainder=0;auto_pass=0;for(u16 i=0;i<8;i++)in_reset(&shown[i]);}
        remainder+=60;while(remainder>=hz){remainder-=hz;tick++;}
        if(demo){samples(tick);for(u16 i=0;i<8;i++){in_update(&shown[i],&simulated[i],hz);in_join(shown,i);}}
        else for(u16 i=0;i<8;i++)shown[i]=live[i];
#ifdef INPUT_AUTOTEST
        if(tick>=600 && demo && submitted>=400) {
            u16 joined=0;for(u16 i=0;i<8;i++)if(shown[i].player)joined++;
            auto_pass=joined==6;
        }
#endif
        draw();publish();lab_wait();
    }
}

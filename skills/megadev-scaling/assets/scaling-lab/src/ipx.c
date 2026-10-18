#include "lab.h"
#include "shared.h"
#include "scale.h"
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
static u16 desired[CELLS],displayed[CELLS],stage,hz;
static u16 dirty_cells[CELLS],dirty_head,dirty_tail,dirty_count;
static unsigned char dirty_mark[CELLS];
static ScRequest request={1,0,2,0};
static u16 automatic=1,paused,clock_remainder;
static volatile u16 dma_offset,front_image,image_count,views_seen,image_rate,rate_tick,rate_base;
extern u16 const scale_palette[];
static Frame frames[2];
static volatile u16 published,submitted,video_fault,input_hits;
extern void scaling_install(void);
#ifdef SCALING_AUTOTEST
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
    if(++rate_tick==hz){image_rate=(u16)(image_count-rate_base);rate_base=image_count;rate_tick=0;}
    u16 index=published;if(!index)return;
    Frame const *f=&frames[index-1];
    if(f->count>DIRTY_CAP || f->sprites>8){video_fault=1;published=0;return;}
    for(u16 i=0;i<f->count;i++)if(f->cells[i].offset>=CELLS){video_fault=1;published=0;return;}
    /* At most 4096 image bytes + 512 map bytes + 384 HUD bytes in
       an early VBlank. A late callback defers the whole image upload. */
    u16 vline=*(volatile u16 *)0xC00008u>>8;
    if(scale_upload && (*(volatile u16 *)0xC00004u&8u) && vline>=224 && vline<=230) {
        u16 tile=(u16)(256u+(front_image^1u)*256u);
        lab_upload_work(scale_pixels+dma_offset,to_vdp_addr(tile*32u+dma_offset*2u)|VRAM_W,SC_UPLOAD_WORDS);
        dma_offset+=SC_UPLOAD_WORDS;
        if(dma_offset==SC_WORDS) {
            for(u16 y=0;y<16;y++) {
                vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(y+4u)*128u+12u*2u)|VRAM_W;
                for(u16 x=0;x<16;x++)vdp_data_16=(u16)(0xA000u+tile+x*16u+y);
            }
            front_image^=1u;image_count++;views_seen|=(u16)(1u<<scale_last_mode);dma_offset=0;
            __asm__ volatile("" ::: "memory");scale_upload=0;
        }
    }
    for(u16 i=0;i<f->count;i++) {
        u16 cell=f->cells[i].offset;
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(cell/40u)*128u+(cell%40u)*2u)|VRAM_W;
        vdp_data_16=f->cells[i].word;displayed[cell]=f->cells[i].word;
    }
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    for(u16 w=0;w<4;w++)vdp_data_16=f->sat[0][w];
    submitted++;published=0;
}
static void text(u16 x,u16 y,u16 color,char const *s) {
    if(color==1)color=2; /* Palette 1 belongs to art, never HUD glyphs. */
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
static void decimal(u16 x,u16 y,u16 value) {
    char s[3]={'0','0',0};if(value>99)value=99;
    while(value>=10){value-=10;s[0]++;}s[1]+=(char)value;text(x,y,1,s);
}
void scaling_draw(void) {
    text(9,0,1,"THE BRASS ORRERY");text(9,1,0,"MEGA CD SCALING LAB");
    text(1,4,0,"VIEW");text(1,5,1,request.mode==0?"ZOOM ":request.mode==1?"ORBIT":"FLOOR");
    text(1,8,0,"HEADING");hex(1,9,1,request.angle);
    text(1,12,0,"SCALE");hex(1,13,1,request.zoom);
    text(29,4,0,"IMAGES");hex(29,5,1,image_count);
    text(29,7,0,"IMAGE/S");decimal(29,8,image_rate);
    text(29,10,0,"UPLOAD");text(29,11,1,"4K/BLANK");
    text(29,13,0,"STATE");text(29,14,1,scale_error?"FAULT ":paused?"PAUSED":automatic?"AUTO  ":"MANUAL");
    text(29,16,0,"ERRORS");hex(29,17,1,scale_error);
    text(1,21,0,"128 X 128 ASIC IMAGE - STAGED VRAM");
    text(1,23,0,"A VIEW  B AUTO  C RESET  START PAUSE");
    text(1,24,0,"LEFT/RIGHT TURN   UP/DOWN SCALE");
    text(1,26,0,"MAIN");hex(6,26,1,main_ticks);text(15,26,0,"SUB");hex(19,26,1,*ga_reg_comstat3);text(34,26,0,hz==50?"PAL":"NTSC");
#ifdef SCALING_AUTOTEST
    text(1,27,1,tour_ok?"AUTO PASS - THREE ASIC VIEWS":"AUTOMATIC THREE VIEW TOUR");
#endif
}
void scaling_publish(void) {
    Frame *f=&frames[stage];f->count=0;f->sprites=0;
    while(dirty_count && f->count<DIRTY_CAP) {
        u16 cell=dirty_cells[dirty_head];dirty_mark[cell]=0;
        dirty_head++;if(dirty_head==CELLS)dirty_head=0;dirty_count--;
        if(desired[cell]!=displayed[cell]){f->cells[f->count].offset=cell;f->cells[f->count].word=desired[cell];f->count++;}
    }
    f->sat[0][0]=(u16)(128+112+scale_sine((u16)(main_ticks*2u))/16);
    f->sat[0][1]=0x0500;f->sat[0][2]=0xC080;f->sat[0][3]=128+276;
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize_video(void) {
    lab_display_off();lab_clear();
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    u16 const colors[4]={0x0EEE,0x06E6,0x06CE,0x068E};
    for(u16 p=0;p<4;p++)for(u16 i=0;i<16;i++)bios_palette[p*16+i]=i==1?(s16)colors[p]:(i==2?0x0220:0);
    bios_vdp_update_flags=0;vdp_ctrl_32=CRAM_W;
    for(u16 i=0;i<16;i++)bios_palette[16+i]=(s16)scale_palette[i];
    for(u16 i=0;i<64;i++)vdp_data_16=(u16)bios_palette[i];
    /* Original 16px foreground wisp, independent of the ASIC output. */
    vdp_ctrl_32=to_vdp_addr(128*32)|VRAM_W;
    for(u16 tx=0;tx<2;tx++)for(u16 ty=0;ty<2;ty++)for(u16 y=0;y<8;y++)for(u16 x=0;x<8;x+=4) {
        u16 word=0;
        for(u16 k=0;k<4;k++) {
            s16 dx=(s16)(tx*8+x+k)-7,dy=(s16)(ty*8+y)-7;
            if(dx<0)dx=-dx;
            if(dy<0)dy=-dy;
            word=(u16)((word<<4)|(dx+dy<7?(dx+dy<3?1:2):0));
        }
        vdp_data_16=word;
    }
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){desired[y*40+x]=displayed[y*40+x]=0x8020;vdp_data_16=0x8020;}
    }
}
void scaling_input(u16 hit) {
    if(hit&PAD_START)paused^=1;
    if(hit&PAD_A){request.mode++;if(request.mode==3)request.mode=0;}
    if(hit&PAD_B)automatic^=1;
    if(hit&PAD_C){request=(ScRequest){1,0,2,0};automatic=1;paused=0;}
    if(hit&PAD_LEFT){request.angle=(request.angle-4u)&255u;automatic=0;}
    if(hit&PAD_RIGHT){request.angle=(request.angle+4u)&255u;automatic=0;}
    if(hit&PAD_UP && request.zoom)request.zoom--;
    if(hit&PAD_DOWN && request.zoom<4)request.zoom++;
}
void main(void) {
    disable_interrupts();scaling_install();
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    initialize_video();scaling_draw();
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){u16 cell=y*40+x;displayed[cell]=desired[cell];vdp_data_16=desired[cell];}
    }
    for(u16 i=0;i<CELLS;i++)dirty_mark[i]=0;
    dirty_head=dirty_tail=dirty_count=0;
    lab_display_on();enable_interrupts();
    u16 previous_tick=main_ticks;
    for(;;) {
        lab_wait();u16 delta=(u16)(main_ticks-previous_tick);previous_tick=main_ticks;
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
        scale_main_tick(delta>=hz?300:(u16)(delta*(hz==50?6u:5u)));
#ifdef SCALING_AUTOTEST
        hit=0;request.mode=(main_ticks/180u)%3u;
        if(main_ticks>540 && image_count>12 && views_seen==7 && !scale_error && !video_fault)tour_ok=1;
#endif
        scaling_input(hit);
        if(!paused) {
            clock_remainder+=(u16)(delta*(hz==50?6u:5u));
            while(clock_remainder>=10u) {
                clock_remainder-=10;request.motion++;
                if(automatic)request.angle=(request.angle+2u)&255u;
            }
            scale_start(&request);
        }
        scaling_draw();scaling_publish();
        /* Recheck actual grant/retirement after foreground UI work; do not
           deliberately hold a completed handshake until another VBlank. */
        scale_main_tick(0);if(!paused)scale_start(&request);
    }
}

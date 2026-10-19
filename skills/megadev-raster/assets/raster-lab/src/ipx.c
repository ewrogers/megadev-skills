#include "lab.h"
#include "shared.h"
#include "raster.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
RtWorld world;
extern u8 const res_font[],res_actors[],res_ground[],res_ground_end[],res_map[],res_reflection[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
#define HUD 256
typedef struct {RtView view;u16 hud[HUD];} Frame;
_Static_assert(sizeof(RtPlan)==20 && sizeof(RtWorld)==30 && sizeof(RtView)==1062 && sizeof(Frame)==1574,"Raster assembly/publication ABI changed");
static Frame frames[2];
static u16 stage,hz,front,front_valid;
static volatile u16 published,input_hits,submitted,video_fault,upload_high,setup_late;
static u16 frame_rate,last_submitted,window_start;
#ifdef RASTER_AUTOTEST
static u16 tour_ok;
#endif
static void reg(u16 index,u16 value){u16 command=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=command;vdp_ctrl_16=command;}
void lab_line(char const *s,u16 x,u16 y){char line[41];u16 n=0;if(x>=40 || y>=4)return;while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u+x*2u)|VRAM_W);}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
void input_user(void){input_hits|=bios_joy1_hit;}
void vblank_user(void){
 main_ticks++;raster_epoch++;if(!raster_epoch)raster_epoch=1;
 raster_armed=0;
 u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232){if(setup_late<65535)setup_late++;return;}
 reg(0,(u16)(bios_vdp_regs[0]&0xEF));raster_reg_off=bios_vdp_regs[0];
 /* Restore before any bulk submission: never carry wet actor colors into
    the top of another frame, including one with no new foreground snapshot. */
 reg(15,2);
 vdp_ctrl_32=0xC0680000u;vdp_data_16=0x4CE;vdp_data_16=0x28E;
 reg(2,0x38);
 u16 next=published,accepted=0;
 if(next){
  if(next>2 || frames[next-1].view.count>RT_SPRITES || !rt_plan_valid(&frames[next-1].view.plan)){video_fault=1;published=0;}
  else {front=(u16)(next-1);front_valid=1;accepted=1;}
 }
 if(!front_valid)return;
 Frame const *f=&frames[front];RtPlan const *p=&f->view.plan;
 raster_plan=*p;reg(10,(u16)(p->line-1));
 vdp_ctrl_32=VSRAM_W;vdp_data_16=p->top_a;vdp_data_16=p->top_b;
 if(accepted){
  lab_upload_work(f->view.scroll,to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W,448);
  lab_upload_work(f->view.sat,to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W,(f->view.count?f->view.count:1)*4u);
  lab_upload_work(f->hud,to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW)|VRAM_W,256);
  u16 bytes=(u16)(896+(f->view.count?f->view.count:1)*8+512+8);if(bytes>upload_high)upload_high=bytes;
 }
 /* Dedicated armed copy stays immutable through active display; Main writes
    only its unpublished Frame. Parameters precede the aligned epoch commit. */
 raster_setup=(u16)(*(volatile u16 *)0xC00008u>>8);
 if(p->enabled){
  if((vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) && raster_setup>=224){raster_armed=raster_epoch;reg(0,(u16)((bios_vdp_regs[0]&255)|16));}
  else if(setup_late<65535)setup_late++;
 }
 if(accepted){submitted++;published=0;}
}
static void text(Frame *f,u16 x,u16 y,char const *s){while(*s && x<40 && y<4){f->hud[y*64+x]=(u16)(0x8000u|(u8)*s);x++;s++;}}
static void number(Frame *f,u16 x,u16 y,u16 n){char s[6];s[5]=0;for(u16 i=5;i;i--){s[i-1]=(!n && i<5)?' ':(char)('0'+n%10);n/=10;}text(f,x,y,s);}
/* One division per measurement window; not a hot-path optimization target. */
static u16 rate(u16 frames_done,u16 blanks){if(!blanks || frames_done>1024)return 0;u16 product=(u16)(frames_done*hz);return (u16)(product/blanks);}
void raster_measure(u16 now){
 u16 elapsed=(u16)(now-window_start);
 if(elapsed>=hz && elapsed){frame_rate=rate((u16)(submitted-last_submitted),elapsed);last_submitted=submitted;window_start=now;}
}
void raster_publish(void){
 if(published)return;
 Frame *f=&frames[stage];rt_build(&world,&f->view);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,world.demo?"SPLIT LAB":"WATER LAB");text(f,22,0,"FPS");number(f,26,0,frame_rate);text(f,34,0,hz==50?"PAL":"NTSC");
 text(f,1,1,world.demo?"D-PAD PAN  A SELECT B SPLIT C TOUR":"LEFT RIGHT ORB  UP DOWN SURFACE");
 text(f,1,2,world.paused?"PAUSED":world.demo?(world.selected?"P2 ACTIVE  START PAUSE":"P1 ACTIVE  START PAUSE"):"A TINT B WAVES C SWEEP START PAUSE");
 text(f,1,3,"H");number(f,2,3,raster_hits);text(f,8,3,"L");number(f,9,3,raster_late);text(f,15,3,"T");number(f,16,3,raster_timeout);text(f,22,3,"S");number(f,23,3,setup_late);

#ifdef RASTER_AUTOTEST
 if(tour_ok)text(f,1,0,"AUTO PASS ");
#endif
 if(video_fault)text(f,1,0,"VIDEO FAULT");
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize(void){
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x400)|VRAM_W,1536);
 lab_upload_work(res_actors,to_vdp_addr(0x1000)|VRAM_W,28*16);
 lab_upload_work(res_ground,to_vdp_addr(0x2000)|VRAM_W,(u32)(res_ground_end-res_ground)/2u);
 bios_vdp_update_flags=0;reg(0,(u16)(bios_vdp_regs[0]&0xEF));reg(4,6);reg(11,3);reg(16,1);reg(17,0);reg(18,4);reg(7,17);reg(15,2);
 vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++){u16 c=(u16)((res_palettes[i*2]<<8)|res_palettes[i*2+1]);bios_palette[i]=c;vdp_data_16=c;}
 vdp_ctrl_32=to_vdp_addr(0xC000)|VRAM_W;for(u16 i=0;i<4096;i+=2)vdp_data_16=(u16)((res_map[i]<<8)|res_map[i+1]);
 vdp_ctrl_32=to_vdp_addr(0xE000)|VRAM_W;for(u16 i=0;i<2048;i++)vdp_data_16=0;
#ifndef RASTER_SPLIT
 vdp_ctrl_32=to_vdp_addr(0x8000)|VRAM_W;for(u16 i=0;i<4096;i+=2)vdp_data_16=(u16)((res_reflection[i]<<8)|res_reflection[i+1]);
#endif
 for(u16 y=0;y<4;y++){vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;for(u16 x=0;x<64;x++)vdp_data_16=0x8020;}
 reg(2,0x38);vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
}
#ifdef RASTER_AUTOTEST
void raster_tour(void){RtWorld w;RtView v;rt_init(&w,world.demo);for(u16 i=0;i<600;i++){rt_advance(&w,hz==50?6:5,0,i==0?32:0);rt_build(&w,&v);if(!rt_plan_valid(&v.plan) || v.count>RT_SPRITES)return;}tour_ok=1;world.automatic=1;}
#endif
void main(void){
 disable_interrupts();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
#ifdef RASTER_SPLIT
 rt_init(&world,1);
#else
 rt_init(&world,0);
#endif
 initialize();system_install();raster_install();enable_interrupts();raster_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;window_start=previous;last_submitted=submitted;
#ifdef RASTER_AUTOTEST
 raster_tour();previous=main_ticks;window_start=previous;last_submitted=submitted;
#endif
 for(;;){
  lab_wait();u16 now=main_ticks,delta=(u16)(now-previous);if(!delta)continue;previous=now;
  /* HBlank also wakes STOP: only the VBlank clock advances the foreground. */
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  rt_advance(&world,delta>4?20:(u16)(delta*(hz==50?6:5)),bios_joy1_hold&15u,hit);
  lab_sound(world.tick,hit&112,world.paused);raster_publish();
  raster_measure(now);
 }
}

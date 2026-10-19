#include "lab.h"
#include "shared.h"
#include "motion.h"
#include "gfx.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
MmGame motion;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
#define ACTOR 0x80
#define GROUND 0x100
#define SPR_CAP 28
#define HUD 160
typedef struct {GfxList sprites;u16 hud[HUD];} Frame;
_Static_assert(sizeof(Frame)==1640,"Publisher ABI changed");
_Static_assert(sizeof(MmGame)==30,"Motion ABI changed");
static Frame frames[2];
static u16 stage,hz;
static volatile u16 published,input_hits,submitted,video_fault,upload_high;
#ifdef MATH_AUTOTEST
static u16 tour_ok;
#endif
static void reg(u16 index,u16 value){u16 w=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=w;vdp_ctrl_16=w;}
void lab_line(char const *s,u16 x,u16 y) {
 char line[41];u16 n=0;if(x>=40 || y>=4)return;
 while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
 lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u+x*2u)|VRAM_W);
}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
void vblank_user(void) {
 main_ticks++;input_hits|=bios_joy1_hit;
 u16 index=published;if(!index)return;
 if(index>2){video_fault=1;published=0;return;}
 Frame const *f=&frames[index-1];u16 count=f->sprites.count;
 if(count>SPR_CAP){video_fault=1;published=0;return;}
 u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232)return;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
 for(u16 i=0;i<(count?count:1);i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sprites.sat[i][w];
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;
  for(u16 x=0;x<40;x++)vdp_data_16=f->hud[y*40u+x];
 }
 u16 bytes=(u16)((count?count:1)*8u+HUD*2u);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s) {
 while(*s && x<40 && y<4){f->hud[y*40+x]=(u16)(0x8000u|(u8)*s);x++;s++;}
}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 palette) {
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,tile,palette,1,0);
}
static void dot(Frame *f,MmS16 x,MmS16 y,u16 kind) {sprite(f,(s16)(156+mm_to_int(x)),(s16)(128+mm_to_int(y)),8,8,(u16)(ACTOR+kind),1);}
void math_publish(void) {
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"MATH LAB");text(f,22,0,motion.view==0?"TRIG / ORBITS":motion.view==1?"UNIT VECTORS":"LERP / EASING");
 text(f,1,1,motion.view==1?"D-PAD BEACON / A AUTO":"UP DOWN SPEED / A REVERSE");
 text(f,1,2,"C VIEW   B RESET   START PAUSE");
 text(f,1,3,motion.paused?"PAUSED":motion.faults?"MATH CHECK FAILED":motion.view==2?"BLUE LINEAR / GOLD SMOOTHSTEP":"Q8.8 / 1024 ANGLE UNITS PER TURN");
 text(f,34,3,hz==50?"PAL":"NTSC");
 if(motion.view==0) {
  for(u16 i=0;i<12;i++) {
   MmVec base={(MmS16)((i&1u)?64*256:40*256),0},p;
   MmU16 angle=(MmU16)(motion.angle+i*85u);
   if(mm_rotate(&base,angle,&p))dot(f,p.x,p.y,(u16)(i&1u));
  }
  dot(f,0,0,3);
 } else if(motion.view==1) {
  dot(f,motion.target.x,motion.target.y,3);dot(f,motion.ship.x,motion.ship.y,0);
  for(u16 i=1;i<9;i++) {
   MmS16 x,y;mm_lerp(motion.ship.x,motion.target.x,(MmU16)(i*28u),&x);mm_lerp(motion.ship.y,motion.target.y,(MmU16)(i*28u),&y);dot(f,x,y,2);
  }
 } else {
  MmS16 linear,eased;MmU16 t;mm_smoothstep(motion.t,&t);mm_lerp(-80*256,80*256,motion.t,&linear);mm_lerp(-80*256,80*256,t,&eased);
  for(u16 i=0;i<9;i++){dot(f,(MmS16)((MmS16)i*20*256-80*256),-24*256,3);dot(f,(MmS16)((MmS16)i*20*256-80*256),24*256,3);}
  dot(f,linear,-24*256,0);dot(f,eased,24*256,1);
 }
#ifdef MATH_AUTOTEST
 if(tour_ok)text(f,1,3,"AUTO PASS / ARITHMETIC MOTION");
#endif
 if(video_fault)text(f,1,3,"VIDEO CHECK FAILED / RESET");
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize(void) {
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,4*16);
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,16);
 bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,4);reg(7,2);
 vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++) {
  u16 c=(u16)((res_palettes[i*2u]<<8)|res_palettes[i*2u+1u]);bios_palette[i]=c;vdp_data_16=c;
 }
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=0;
 for(u16 y=0;y<32;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128u)|VRAM_W;for(u16 x=0;x<64;x++)vdp_data_16=0;
 }
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;for(u16 x=0;x<64;x++)vdp_data_16=0x8020;
 }
 for(u16 y=4;y<28;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+y*128u)|VRAM_W;
  for(u16 x=0;x<40;x++)vdp_data_16=GROUND|0x4000u;
 }
}
#ifdef MATH_AUTOTEST
void math_tour(void) {
 for(u16 view=0;view<3;view++) {
  mt_init(&motion);motion.view=view;for(u16 i=0;i<300;i++)mt_advance(&motion,5,0,0);
  if(motion.faults)return;
 }
 tour_ok=1;motion.paused=1;
}
#endif
void main(void) {
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 mt_init(&motion);initialize();enable_interrupts();math_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;
 for(;;) {
  lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  mt_advance(&motion,delta>=hz?300:(u16)(delta*(hz==50?6u:5u)),bios_joy1_hold&15u,hit);
#ifdef MATH_AUTOTEST
  if(!tour_ok)math_tour();
#endif
  lab_sound(motion.tick,hit&16?2:0,motion.paused);
  math_publish();
 }
}

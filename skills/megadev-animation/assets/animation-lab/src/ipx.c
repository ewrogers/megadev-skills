#include "lab.h"
#include "shared.h"
#include "training.h"
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
Training training;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 event,u16 paused);
#define ACTOR 0x80
#define GROUND 0x180
#define SPR_CAP 16
#define HUD 160
_Static_assert(ACTOR+190<=GROUND && (GROUND+10)*32u<BIOS_VDP_DEFAULT_WINDOW,"VRAM overlaps");
_Static_assert(sizeof(Training)==48 && sizeof(AnPlayer)==10,"Model ABI changed");
typedef struct {GfxList sprites;u16 hud[HUD];} Frame;
_Static_assert(sizeof(Frame)==1640,"Frame ABI changed");
static Frame frames[2];
static u16 stage,hz;
static volatile u16 published,input_hits,video_fault,submitted,upload_high;
#ifdef ANIMATION_AUTOTEST
static u16 tour_step,tour_ok;
#endif
static void reg(u16 index,u16 value){u16 word=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=word;vdp_ctrl_16=word;}
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
static void number(Frame *f,u16 x,u16 y,u16 n){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(n>>(12-i*4))&15u];s[4]=0;text(f,x,y,s);}
static void digit(Frame *f,u16 x,u16 y,u16 n){char s[2]={(char)('0'+n),0};text(f,x,y,s);}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 flip) {
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,tile,1,1,flip);
}
void animation_publish(void) {
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"ANIMATION LAB");text(f,22,0,training.paused?"PAUSED":training.slow?"QUARTER SPEED":"CLOCKWORK YARD");
 AnFrame const *pose=an_pose(&training.player);
 if(!pose){video_fault=1;return;}
 text(f,1,1,an_clips[training.player.clip].name);text(f,10,1,"FRAME");digit(f,16,1,training.player.frame+1);
 text(f,18,1,"OF");digit(f,21,1,an_clips[training.player.clip].count);text(f,25,1,"HITS");number(f,30,1,training.hits);
 text(f,1,2,training.player.clip==AN_DEATH?(training.player.finished?"A RISE / LAST POSE HELD":"FALLING / WAIT FOR FINAL POSE"):"A ATTACK  B HURT  C BOXES");
 text(f,34,2,hz==50?"PAL":"NTSC");
 text(f,1,3,"DOWN+B FALL  UP SLOW  START PAUSE");
 if(video_fault)text(f,1,3,"VIDEO CHECK FAILED / RESET DISC");
#ifdef ANIMATION_AUTOTEST
 if(tour_ok)text(f,1,3,"AUTO PASS / EVENTS + INTERRUPT");
#endif
 s16 x=(s16)(training.x>>2);
 /* The body and weapon share the same simulation pose; no renderer clock. */
 if(pose) {
  sprite(f,x-8,122,16,24,(u16)(ACTOR+pose->pose*6u),training.left);
  if(pose->weapon!=AN_NONE)sprite(f,training.left?x-34:x+2,118,32,32,(u16)(ACTOR+42u+pose->weapon*16u),training.left);
 }
 u16 dummy=training.dummy_flash?(training.dummy_flash&4u?1:2):0;
 sprite(f,224,120,16,24,(u16)(ACTOR+90u+dummy*6u),0);
 if(training.dummy_flash)sprite(f,240,118,8,8,(u16)(ACTOR+188u+((training.dummy_flash>>2)&1u)),0);
 if(training.debug) {
  sprite(f,x-5,128,32,32,ACTOR+108u+3u*16u,0);
  sprite(f,225,124,32,32,ACTOR+108u+4u*16u,0);
  AnBox hit;
  if(an_hitbox(&training.player,training.left,x,144,&hit))sprite(f,hit.x,hit.y,32,32,(u16)(ACTOR+108u+pose->weapon*16u),0);
 }
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize(void) {
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,190*16);
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,10*16);
 bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,4);reg(7,2);
 vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++) {
  u16 c=(u16)((res_palettes[i*2u]<<8)|res_palettes[i*2u+1u]);bios_palette[i]=c;vdp_data_16=c;
 }
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=0;
 for(u16 y=0;y<32;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++)vdp_data_16=0;
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++) {
   u16 tile=y==6?2u+(x&1u):y==7?4u+(x&1u):y>=17 && y<=19?6u+(x&1u):((x*3u+y*5u)&15u)==0?1:0;
   vdp_data_16=(u16)(GROUND+tile+0x4000u);
  }
 }
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;for(u16 x=0;x<64;x++)vdp_data_16=0x8020;
 }
}
#ifdef ANIMATION_AUTOTEST
void animation_tour(void) {
 if(tour_step==0){training.x=812;tr_tick(&training,0,TR_A);tour_step=1;}
 else if(tour_step==1 && training.player.clip==AN_IDLE){tr_tick(&training,0,TR_A);tour_step=2;}
 else if(tour_step==2 && training.player.clip==AN_ATTACK && training.player.frame==0 && training.player.age>=2){tr_tick(&training,0,TR_B);tour_step=3;}
 else if(tour_step==3 && training.player.clip==AN_IDLE){tr_tick(&training,0,TR_B|TR_DOWN);tour_step=4;}
 else if(tour_step==4 && training.player.finished){tr_tick(&training,0,TR_A);tour_step=5;}
 else if(tour_step==5 && training.player.clip==AN_IDLE){tour_ok=training.hits==1 && training.interrupts==2 && training.completions>=4 && !video_fault;training.paused=1;tour_step=6;}
}
#endif
void main(void) {
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 tr_init(&training);initialize();enable_interrupts();animation_publish();
 do{lab_wait();}while(published);lab_display_on();
 u16 previous=main_ticks;
 for(;;) {
  lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  u16 elapsed=delta>=hz?300:(u16)(delta*(hz==50?6u:5u));
  tr_advance(&training,elapsed,bios_joy1_hold&15u,hit);
#ifdef ANIMATION_AUTOTEST
  animation_tour();
#endif
  lab_sound(training.tick,training.events,training.paused);
  animation_publish();
 }
}

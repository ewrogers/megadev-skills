#include "lab.h"
#include "shared.h"
#include "demo.h"
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
CcDemo demo;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[],res_maps[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
#define ACTOR 0x80
#define GROUND 0x100
#define SPR_CAP 32
#define HUD 160
typedef struct {GfxList sprites;u16 hud[HUD];} Frame;
_Static_assert(sizeof(Frame)==1640,"Publisher ABI changed");
_Static_assert(sizeof(CcDemo)==74 && sizeof(CcHit)==18 && sizeof(CcMove)==24,"Collision ABI changed");
static Frame frames[2];
static u16 stage,hz,drawn_map=65535;
static volatile u16 published,input_hits,submitted,video_fault,upload_high;
/* Only Shots changes a single wall column. Staged with the pending frame. */
static u16 wall_words[24];
static volatile u16 wall_pending;
#ifdef COLLISION_AUTOTEST
static u16 tour_step,tour_ok;
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
 u16 patch_bytes=0;
 if(wall_pending) {
  for(u16 y=0;y<24;y++) {
   vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+(y+4u)*128u+20u*2u)|VRAM_W;
   vdp_data_16=wall_words[y];
  }
  patch_bytes=48;wall_pending=0;
 }
 u16 bytes=(u16)((count?count:1)*8u+HUD*2u+patch_bytes);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s) {
 while(*s && x<40 && y<4){f->hud[y*40+x]=(u16)(0x8000u|(u8)*s);x++;s++;}
}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 palette) {
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,tile,palette,1,0);
}
static void marker(Frame *f,s16 x,s16 y,s16 nx,s16 ny) {
 sprite(f,x-4,y-4,8,8,ACTOR+11,1);
 if(nx)sprite(f,(s16)(x+nx*16-4),y-4,8,8,(u16)(ACTOR+(nx<0?7:8)),1);
 if(ny)sprite(f,x-4,(s16)(y+ny*16-4),8,8,(u16)(ACTOR+(ny<0?9:10)),1);
}
void collision_publish(void) {
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 static char const * const views[]={"1 SHOTS","2 RAYS","3 SLIDING"};
 text(f,1,0,"COLLISION LAB");text(f,25,0,views[demo.mode]);
 if(demo.mode==0) {
  text(f,1,1,"PINK DISCRETE");text(f,21,1,"BLUE SWEPT");
  text(f,1,2,"A FIRE  B SPEED");text(f,17,2,demo.speed==0?"8":demo.speed==1?"64":"128");text(f,23,2,"UP LIFT WALL");
  text(f,1,3,demo.paused?"PAUSED":demo.hit.kind==CC_HIT?"SWEPT STOPPED AT WALL":"C VIEW  START PAUSE");
  sprite(f,24,100,8,8,'D',0);sprite(f,24,164,8,8,'S',0);
  sprite(f,(s16)((demo.discrete.x>>4)-1),100,8,8,ACTOR,1);
  sprite(f,(s16)((demo.swept.x>>4)-1),164,8,8,ACTOR+1,1);
  if(demo.hit.kind==CC_HIT)marker(f,164,168,demo.hit.nx,demo.hit.ny);
 } else if(demo.mode==1) {
  text(f,1,1,"D-PAD AIM   A RESET");text(f,1,2,"C VIEW   START PAUSE");
  text(f,1,3,demo.paused?"PAUSED":demo.hit.kind==CC_HIT?"NEAREST CONTACT + BLOCKED AXIS":"CLEAR LINE TO CURSOR");
  CcS16 ex=demo.aim.x,ey=demo.aim.y;
  if(demo.hit.kind==CC_HIT) {
   ex=(CcS16)(768+cd_fraction((CcS16)(ex-768),demo.hit.num,demo.hit.den));
   ey=(CcS16)(2304+cd_fraction((CcS16)(ey-2304),demo.hit.num,demo.hit.den));
   marker(f,(s16)(ex>>4),(s16)(ey>>4),demo.hit.nx,demo.hit.ny);
  }
  sprite(f,44,140,8,8,ACTOR+1,1);sprite(f,(s16)((demo.aim.x>>4)-4),(s16)((demo.aim.y>>4)-4),8,8,ACTOR+6,1);
  /* Critical markers precede decorative ray dots. Twelve dots bound scanline use. */
  for(u16 i=1;i<13;i++) {
   s16 x=(s16)((768+cd_fraction((CcS16)(ex-768),i,12))>>4);
   s16 y=(s16)((2304+cd_fraction((CcS16)(ey-2304),i,12))>>4);
   sprite(f,x-4,y-4,8,8,ACTOR+12,1);
  }
 } else {
  text(f,1,1,"D-PAD MOVE  A RESET");text(f,1,2,"B SPEED");text(f,9,2,demo.speed==0?"2":demo.speed==1?"8":"32");text(f,15,2,"C VIEW  START PAUSE");
  text(f,1,3,demo.paused?"PAUSED":demo.hit.kind==CC_HIT?"SLIDING / CONTACT":demo.hit.kind>=CC_OVERLAP?"BAD START / A RESET":"FREE MOVEMENT");
  s16 x=(s16)(demo.player.x>>4),y=(s16)(demo.player.y>>4);
  sprite(f,x-2,y-2,16,16,ACTOR+2,1);
  if(demo.hit.kind==CC_HIT) {
   s16 cx=(s16)(x+(demo.hit.nx<0?12:demo.hit.nx>0?0:6));
   s16 cy=(s16)(y+(demo.hit.ny<0?12:demo.hit.ny>0?0:6));marker(f,cx,cy,demo.hit.nx,demo.hit.ny);
  }
 }
 text(f,34,3,hz==50?"PAL":"NTSC");
 if(video_fault)text(f,1,3,"VIDEO CHECK FAILED / RESET");
#ifdef COLLISION_AUTOTEST
 if(tour_ok)text(f,1,3,"AUTO PASS / SWEEP RAY SLIDE");
#endif
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void wall_prepare(u16 map) {
 for(u16 y=0;y<24;y++) {
  u16 at=(u16)(map*1920u+(y*40u+20u)*2u);
  wall_words[y]=(u16)(GROUND+((res_maps[at]<<8)|res_maps[at+1u])+0x4000u);
 }
 wall_pending=1;drawn_map=map;
}
static void map_install(void) {
 lab_display_off();u16 map=cd_map(&demo);u16 offset=(u16)(map*1920u);
 for(u16 y=0;y<24;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+(y+4u)*128u)|VRAM_W;
  for(u16 x=0;x<40;x++) {
   u16 at=(u16)(offset+(y*40u+x)*2u),tile=(u16)((res_maps[at]<<8)|res_maps[at+1u]);
   vdp_data_16=(u16)(GROUND+tile+0x4000u);
  }
 }
 drawn_map=map;
}
static void initialize(void) {
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,13*16);
 extern u8 const res_ground_end[];
 u16 words=(u16)(res_ground_end-res_ground)/2u;
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,words);
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
 map_install();
}
#ifdef COLLISION_AUTOTEST
void collision_tour(void) {
 if(tour_step==0 && demo.discrete_done && demo.swept_done) {
  if(demo.discrete_hit || demo.hit.kind!=CC_HIT)return;
  cd_init(&demo,1);cd_tick(&demo,0);if(demo.hit.kind!=CC_HIT)return;tour_step=1;
 } else if(tour_step==1) {
  cd_init(&demo,2);for(u16 i=0;i<20;i++)cd_tick(&demo,CD_RIGHT|CD_DOWN);
  CcU16 count;CcBox const *walls=cd_walls(3,&count);u16 good=1;
  for(u16 i=0;i<count;i++)if(cc_overlap(demo.player,walls[i]))good=0;
  tour_ok=good && demo.hit.kind!=CC_INVALID && demo.hit.kind!=CC_EXHAUSTED && !video_fault;
  demo.paused=1;tour_step=2;
 }
}
#endif
u16 collision_update(u16 delta,u16 held,u16 hit) {
  cd_advance(&demo,delta>=hz?300:(u16)(delta*(hz==50?6u:5u)),held,hit);
#ifdef COLLISION_AUTOTEST
  collision_tour();
#endif
  lab_sound(demo.tick,demo.events,demo.paused);
  if(published)return 0;
  u16 map=cd_map(&demo),hide=0;
  if(drawn_map!=map) {
   if(drawn_map<2 && map<2)wall_prepare(map);
   else {map_install();hide=1;}
  }
  collision_publish();
  return hide;
}
void main(void) {
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 cd_init(&demo,0);initialize();enable_interrupts();collision_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;
 for(;;) {
  lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  if(collision_update(delta,bios_joy1_hold&15u,hit)) {
   do{lab_wait();}while(published);lab_display_on();previous=main_ticks;
  }
 }
}

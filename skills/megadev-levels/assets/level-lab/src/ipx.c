#include "lab.h"
#include "shared.h"
#include "levels.h"
#include "gfx.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
LvWorld world;
LvGame game;
extern u8 const res_font[],res_actors[],res_ground[],res_ground_end[],res_palettes[],res_world[],res_world_end[],res_attrs[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
#define ACTOR 0x80
#define GROUND 0x100
#define SPR_CAP 8
#define HUD 160
typedef struct {GfxList sprites;u16 hud[HUD],x,y;LvPatch patch;} Frame;
_Static_assert(sizeof(Frame)==2064 && sizeof(LvGame)==34 && sizeof(LvWorld)==58,"Level ABI changed");
_Static_assert(ACTOR+49<=GROUND && (GROUND+256)*32u<BIOS_VDP_DEFAULT_WINDOW,"VRAM overlaps");
static Frame frames[2];
static u16 stage,hz,cached_area=65535,cached_debug,committed_x,committed_y,reload_pending;
static volatile u16 published,input_hits,video_fault,submitted,upload_high;
#ifdef LEVEL_AUTOTEST
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
static u16 patch_valid(LvPatch const *p) {
 if(p->count>LV_RUNS || p->used>LV_WORDS)return 0;
 u16 offset=0;
 for(u16 i=0;i<p->count;i++) {
  LvRun const *r=&p->runs[i];u16 base=r->address&0xF000u;
  if(base!=0xC000u && base!=0xE000u)return 0;
  if(r->offset!=offset || (r->address&1u))return 0;
  if(r->step==128){if(r->count!=32 || (r->address&0x0F80u))return 0;}
  else if(r->step==2){if(r->count!=64 || (r->address&127u))return 0;}
  else return 0;
  offset=(u16)(offset+r->count);if(offset>p->used)return 0;
 }
 return offset==p->used;
}
void vblank_user(void) {
 main_ticks++;input_hits|=bios_joy1_hit;
 u16 index=published;if(!index)return;
 if(index>2){video_fault=1;published=0;return;}
 Frame const *f=&frames[index-1];u16 count=f->sprites.count;
 if(count>SPR_CAP || !patch_valid(&f->patch)){video_fault=1;published=0;return;}
 u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232)return;
 /* Rows/columns are immutable alongside their camera and sprite snapshot. */
 for(u16 i=0;i<f->patch.count;i++) {
  LvRun const *r=&f->patch.runs[i];reg(15,r->step);
  vdp_ctrl_32=to_vdp_addr(r->address)|VRAM_W;
  for(u16 j=0;j<r->count;j++)vdp_data_16=f->patch.words[r->offset+j];
 }
 reg(15,2);
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
 for(u16 i=0;i<(count?count:1);i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sprites.sat[i][w];
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=(u16)(0u-f->x)&1023u;vdp_data_16=(u16)(0u-f->x)&1023u;
 vdp_ctrl_32=VSRAM_W;vdp_data_16=(u16)(f->y-32u)&1023u;vdp_data_16=(u16)(f->y-32u)&1023u;
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;
  for(u16 x=0;x<40;x++)vdp_data_16=f->hud[y*40u+x];
 }
 committed_x=f->x;committed_y=f->y;
 u16 bytes=(u16)((count?count:1)*8u+HUD*2u+8u+f->patch.used*2u);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s) {
 while(*s && x<40 && y<4){f->hud[y*40+x]=(u16)(0x8000u|(u8)*s);x++;s++;}
}
static u16 follow(u16 current,u16 target){return target>current?(target-current>8?(u16)(current+8u):target):(current-target>8?(u16)(current-8u):target);}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile) {
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,tile,1,0,0);
}
void level_publish(void) {
 if(published || !world.count)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 f->x=follow(committed_x,game.camera_x);f->y=follow(committed_y,game.camera_y);
 if(!lv_patch(&f->patch,&world.areas[game.area],res_attrs,committed_x,committed_y,f->x,f->y,game.debug)){video_fault=1;return;}
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"LEVEL LAB");text(f,17,0,game.area?"MOONWATER COURT":"SAFFRON ORCHARD");
 text(f,1,1,"D-PAD WALK   C CAMERA TOUR");text(f,34,1,hz==50?"PAL":"NTSC");
 text(f,1,2,"A TRAVEL   B COLLISION   START PAUSE");
 text(f,1,3,game.paused?"PAUSED":game.debug?"COLLISION VIEW / B TO RETURN":game.near!=65535?"GOLD WAYSTONE / A TO TRAVEL":"FOLLOW THE PATH TO A GOLD WAYSTONE");
 if(video_fault)text(f,1,3,"VIDEO CHECK FAILED / RESET");
#ifdef LEVEL_AUTOTEST
 if(tour_ok)text(f,1,3,"AUTO PASS / MAP CAMERA EXIT");
#endif
 u16 pose=(u16)(game.face*2u+(game.moving?((game.tick>>3)&1u):0u));
 sprite(f,(s16)(game.x-f->x-8u),(s16)(game.y-f->y+12u),16,24,(u16)(ACTOR+pose*6u));
 /* Waystones are presentation; imported object rectangles own interaction. */
 LvArea const *a=&world.areas[game.area];u16 cells=(u16)(a->w*a->h);
 u8 const *p=a->data+20u+(u32)cells*5u+(cells&1u)+a->spawns*8u;
 for(u16 i=0;i<a->exits;i++,p+=16) {
  u16 x=(u16)((p[2]<<8)|p[3]),y=(u16)((p[4]<<8)|p[5]),w=(u16)((p[6]<<8)|p[7]),h=(u16)((p[8]<<8)|p[9]);
  sprite(f,(s16)(x+w/2u-f->x-4u),(s16)(y+h/2u-f->y+28u-((game.tick>>4)&1u)),8,8,ACTOR+48);
 }
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void install_map(void) {
 lab_display_off();LvArea const *a=&world.areas[game.area];u16 left=game.camera_x>>3,top=game.camera_y>>3;
 for(u16 plane=0;plane<2;plane++)for(u16 py=0;py<32;py++) {
  u16 wy=(u16)(top+((py+32u-(top&31u))&31u));
  vdp_ctrl_32=to_vdp_addr((plane?BIOS_VDP_DEFAULT_PLANEA:BIOS_VDP_DEFAULT_PLANEB)+py*128u)|VRAM_W;
  for(u16 px=0;px<64;px++) {
   u16 wx=(u16)(left+((px+64u-(left&63u))&63u));vdp_data_16=lv_word(a,res_attrs,plane,wx,wy,game.debug);
  }
 }
 committed_x=game.camera_x;committed_y=game.camera_y;cached_area=game.area;cached_debug=game.debug;reload_pending=0;
}
u16 level_update(u16 delta,u16 held,u16 hit) {
 lv_advance(&world,&game,delta>=hz?300:(u16)(delta*(hz==50?6u:5u)),held,hit);
 if(game.events&(LV_EVENT_TRAVEL|LV_EVENT_WARP))reload_pending=1;
 lab_sound(game.tick,game.events,game.paused);
 if(published)return 0;
 u16 changed=cached_area!=game.area || cached_debug!=game.debug || reload_pending;
 if(changed)install_map();
 level_publish();return changed;
}
#ifdef LEVEL_AUTOTEST
void level_tour(void) {
 if(tour_step<5){lv_spawn(&world,&game,0,tour_step);reload_pending=1;tour_step++;return;}
 if(tour_step==5){game.x=736;game.y=264;lv_advance(&world,&game,5,0,LV_USE);reload_pending=1;tour_step++;}
 if(tour_step==6 && game.area==1){tour_ok=game.visits==1 && !lv_blocked(&world.areas[1],(s16)game.x,(s16)game.y) && !video_fault;game.paused=1;tour_step++;}
}
#endif
void level_configure(void) {
 /* H64/V32 ring, matching both the importer cache model and 0x1000 tables. */
 bios_vdp_update_flags=0;reg(11,0);reg(16,1);reg(17,0);reg(18,4);reg(7,2);reg(15,2);
}
void main(void) {
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 if(!lv_open(&world,res_world,(u32)(res_world_end-res_world)))for(;;)__asm__ volatile("nop");
 lv_init(&world,&game);lab_display_off();lab_clear();
 /* Tile zero is transparent world space; the font SPACE tile is opaque HUD. */
 vdp_ctrl_32=to_vdp_addr(0)|VRAM_W;for(u16 i=0;i<16;i++)vdp_data_16=0;
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,49*16);
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,(u16)(res_ground_end-res_ground)/2u);
 level_configure();
 vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++){u16 c=(u16)((res_palettes[i*2u]<<8)|res_palettes[i*2u+1u]);bios_palette[i]=c;vdp_data_16=c;}
 install_map();enable_interrupts();level_publish();do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;
 for(;;) {
  lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
#ifdef LEVEL_AUTOTEST
  if(!published)level_tour();
#endif
  if(level_update(delta,bios_joy1_hold&15u,hit)){do{lab_wait();}while(published);lab_display_on();previous=main_ticks;}
 }
}

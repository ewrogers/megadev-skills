#include "lab.h"
#include "shared.h"
#include "link.h"
#include "gameplay.h"
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
Gameplay game;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 event,u16 paused);
#define ACTOR 0x80
#define GROUND 0x100
#define SPR_CAP 28
#define HUD 160
/* 92 sprite patterns + ten ground patterns; tables remain at BIOS defaults. */
_Static_assert(ACTOR+92<=GROUND && (GROUND+10)*32u<BIOS_VDP_DEFAULT_WINDOW,"VRAM overlaps");
typedef struct {GfxList sprites;u16 hud[HUD],x,y;} Frame;
_Static_assert(sizeof(Frame)==1644,"Frame ABI changed");
_Static_assert(sizeof(Gameplay)==172,"Gameplay ABI changed");
static Frame frames[2];
static u16 stage,hz,drawn_area=2;
static volatile u16 published,input_hits,video_fault,submitted,upload_high;
#ifdef GAMEPLAY_AUTOTEST
static u16 tour_ok;
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
 /* If service entry is late, keep the immutable frame for the next blank. */
 u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232)return;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
 for(u16 i=0;i<(count?count:1);i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sprites.sat[i][w];
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=(u16)(0u-f->x)&1023u;
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=(u16)(f->y-32u)&1023u;
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;
  for(u16 x=0;x<40;x++)vdp_data_16=f->hud[y*40u+x];
 }
 u16 bytes=(u16)((count?count:1)*8u+8u+HUD*2u);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,u16 color,char const *s) {
 while(*s && x<40 && y<4){f->hud[y*40+x]=(u16)(0x8000u|(color<<13)|(u8)*s);x++;s++;}
}
static void digit(Frame *f,u16 x,u16 y,u16 n){char s[2]={(char)('0'+n),0};text(f,x,y,0,s);}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 flip) {
 /* Do not let actors cover the opaque Window HUD. Admission still accounts
    for total entries and pixels on every visible H40 scanline. */
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,tile,1,1,flip);
}
void gameplay_publish(void) {
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);f->x=game.camera_x;f->y=game.camera_y;
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,0,"ADVENTURE LAB");text(f,22,0,0,game.area?"LANTERN COURT":"GLASS GARDEN");
 text(f,1,1,0,"HEARTS");for(u16 i=0;i<game.hp;i++)text(f,8+i*2u,1,1,"+");
 u16 gems=0;for(u16 a=0;a<2;a++)for(u16 i=0;i<6;i++)if(game.gems[a]&(1u<<i))gems++;
 text(f,16,1,0,"GEMS");digit(f,21,1,gems>=10?1:0);digit(f,22,1,gems>=10?gems-10:gems);text(f,23,1,0,"/12");text(f,33,1,0,hz==50?"PAL":"NTSC");
 text(f,1,2,0,"PULSE");for(u16 i=0;i<8;i++)text(f,7+i,2,0,game.cooldown<=i*24u?"+":"-");
 text(f,17,2,0,game.paused?"PAUSED":game.complete?"ALL GEMS FOUND!":client.phase==SY_IDLE?"C SUB JOB / READY":"SUB WORKS / KEEP MOVING");
 text(f,1,3,0,"A PULSE  B RESET  START PAUSE");
 if(verify_failed || video_fault)text(f,1,3,0,"CHECK FAILED / RESET DISC");
#ifdef GAMEPLAY_AUTOTEST
 if(tour_ok)text(f,1,3,0,"AUTO PASS / MAIN + SUB");
#endif
 /* Gameplay first, cosmetic effects last; overload cannot remove collision. */
 s16 px=(s16)((game.x>>2)-game.camera_x-8),py=(s16)((game.y>>2)-game.camera_y+14);
 u16 frame=game.face*2u+(game.moving?((game.tick>>3)&1u):0u);
 if(!game.hurt || (game.tick&4u))sprite(f,px,py,16,24,(u16)(ACTOR+frame*6u),0);
 for(u16 i=0;i<GP_ENEMIES;i++) {
  GpEnemy const *e=&game.enemies[i];s16 x=(s16)((e->x>>2)-game.camera_x-8),y=(s16)((e->y>>2)-game.camera_y+20);
  sprite(f,x,y,16,16,(u16)(ACTOR+48u+((game.tick>>4)&1u)*4u),e->direction);
  if(e->stun)sprite(f,x+4,y-8,8,8,ACTOR+90,0);
 }
 for(u16 i=0;i<GP_ITEMS;i++)if(!(game.gems[game.area]&(1u<<i))) {
  s16 x=(s16)((gp_item_x(game.area,i)>>2)-game.camera_x-4),y=(s16)((gp_item_y(game.area,i)>>2)-game.camera_y+24);
  sprite(f,x,y,8,8,(u16)(ACTOR+56u+((game.tick>>4)&1u)),0);
 }
 if(game.pulse)sprite(f,px-8,py-2,32,32,(u16)(ACTOR+58u+((game.tick>>2)&1u)*16u),0);
 for(u16 i=0;i<GP_EFFECTS;i++)if(game.effects[i].age) {
  GpEffect const *e=&game.effects[i];sprite(f,(s16)((e->x>>2)-game.camera_x-4),(s16)((e->y>>2)-game.camera_y+16-((24u-e->age)>>2)),8,8,(u16)(ACTOR+90u+(e->age<12)),0);
 }
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void area_map(void) {
 /* This small Lab keeps both collision layouts and all art in Main RAM.
    No live Word RAM read, no CD load and no allocation on an area crossing. */
 lab_display_off();
 for(u16 y=0;y<32;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++) {
   u16 wall=gp_solid(game.area,(s16)(x>>1),(s16)(y>>1)),t;
   if(wall)t=2u+(x&1u)+(y&1u)*2u;
   else if((x<3 || x>60) && y>=14 && y<=17)t=x<3?8:9;
   else if(y>=14 && y<=17)t=6u+(x&1u);
   else t=((x*3u+y*5u)&15u)==0?1:0;
   vdp_data_16=(u16)(GROUND+t+((game.area+2u)<<13));
  }
 }
 drawn_area=game.area;
}
static void initialize(void) {
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,92*16);
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,10*16);
 bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,4);reg(7,2);
 vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++) {
  u16 c=(u16)((res_palettes[i*2u]<<8)|res_palettes[i*2u+1u]);bios_palette[i]=c;vdp_data_16=c;
 }
 for(u16 y=0;y<32;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128u)|VRAM_W;for(u16 x=0;x<64;x++)vdp_data_16=0;
 }
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;for(u16 x=0;x<64;x++)vdp_data_16=0x8020;
 }
 area_map();
}
#ifdef GAMEPLAY_AUTOTEST
void gameplay_tour(void) {
 static u16 step;
 if(step==0){sy_main_start(0,0);step=1;}
 if(step==1 && passed && client.phase==SY_IDLE){game.x=1996;game.y=512;gp_advance(&game,5,GP_RIGHT,0);step=2;}
 if(step==2 && game.area==1){sy_main_start(0,1);step=3;}
 if(step==3 && drained && client.phase==SY_IDLE){tour_ok=game.visits==1 && game.tick>100 && !verify_failed && !video_fault;step=4;}
}
#endif
void main(void) {
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 gp_init(&game);initialize();enable_interrupts();gameplay_publish();
 do{lab_wait();}while(published);lab_display_on();
 u16 previous=main_ticks;
 for(;;) {
  lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  u16 elapsed=delta>=hz?300:(u16)(delta*(hz==50?6u:5u));
  sy_main_tick(elapsed); /* Service deadlines continue during gameplay pause. */
  gp_advance(&game,elapsed,bios_joy1_hold&15u,hit);
  lab_sound(game.tick,game.events,game.paused);
  if(hit&PAD_C)sy_main_start(0,0);
#ifdef GAMEPLAY_AUTOTEST
  gameplay_tour();
#endif
  /* Never rewrite a published snapshot or touch VDP while its callback owns it. */
  if(published)continue;
  u16 switched=drawn_area!=game.area;
  if(switched)area_map();
  gameplay_publish();
  if(switched){do{lab_wait();}while(published);lab_display_on();previous=main_ticks;}
 }
}

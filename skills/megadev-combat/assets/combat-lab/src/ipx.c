#include "lab.h"
#include "shared.h"
#include "combat.h"
#include "art_meta.h"
#include "gfx.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
CoWorld world;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
extern int combat_contract(void);
#define ACTOR 0x80u
#define GROUND 0x280u
#define SPR_CAP 56u
#define HUD 160u
_Static_assert(ACTOR+ART_COUNT<=GROUND && (GROUND+GROUND_COUNT)*32u<BIOS_VDP_DEFAULT_WINDOW,"Art overlaps VDP tables");
typedef struct {GfxList sprites;u16 hud[HUD],overlay[160],overlay_kind;} Frame;
static Frame frames[2];
static u16 stage,hz,shown_overlay;
static volatile u16 published,input_hits,input_hits2,submitted,video_fault,upload_high;
static u16 tour_result;
static void reg(u16 index,u16 value){u16 w=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=w;vdp_ctrl_16=w;}
void lab_line(char const *s,u16 x,u16 y){
 char line[41];u16 n=0;if(x>=40 || y>=4)return;
 while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xff;
 lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u+x*2u)|VRAM_W);
}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
void combat_input_latch(void){input_hits|=bios_joy1_hit;input_hits2|=bios_joy2_hit;}
void vblank_user(void){
 main_ticks++;u16 index=published;if(!index)return;
 if(index>2){video_fault=1;published=0;return;}
 Frame const *f=&frames[index-1u];u16 count=f->sprites.count;
 if(count>SPR_CAP){video_fault=1;published=0;return;}
 u16 vertical=(u16)(*(volatile u16 *)0xc00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232)return;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
 for(u16 i=0;i<(count?count:1);i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sprites.sat[i][w];
 for(u16 y=0;y<4;y++){
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;
  for(u16 x=0;x<40;x++)vdp_data_16=f->hud[y*40u+x];
 }
 u16 panel_bytes=0;
 if(f->overlay_kind!=shown_overlay){
  for(u16 y=0;y<5;y++){
   vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(y+13u)*128u+8u)|VRAM_W;
   for(u16 x=0;x<32;x++)vdp_data_16=f->overlay[y*32u+x];
  }
  shown_overlay=f->overlay_kind;panel_bytes=320;
 }
 u16 bytes=(u16)((count?count:1)*8u+HUD*2u+panel_bytes);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s){while(*s && x<40 && y<4)f->hud[y*40u+x++]=(u16)(0x8000u|(u8)*s++);}
static void icon(Frame *f,u16 x,u16 y,u16 tile){f->hud[y*40u+x]=(u16)(0xa000u+ACTOR+tile);}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 palette){
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,(u16)(ACTOR+tile),palette,(world.help || world.complete || world.game_over)?0:1,0);
}
static void panel(Frame *f,u16 row,u16 col,char const *s){while(*s && col<32 && row<5)f->overlay[row*32u+col++]=(u16)(0x8000u|(u8)*s++);}
void combat_publish(void){
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"COMBAT LAB / COPPERLEAF COURT");text(f,34,0,hz==50?"PAL":"NTSC");
 text(f,1,1,"P1");for(u16 i=0;i<5;i++)icon(f,4+i,1,i<world.actor[0].hp?ART_HEART:ART_EMPTY);
 text(f,12,1,"P2");
 if(world.actor[1].joined){for(u16 i=0;i<5;i++)icon(f,15+i,1,i<world.actor[1].hp?ART_HEART:ART_EMPTY);}
 else text(f,15,1,"START TO JOIN");
 text(f,1,2,world.paused?"PAUSED":world.complete?"COURT CLEAR! B TRY AGAIN":world.game_over?"REST AND RETRY / B":world.help?"A / START TO BEGIN":"STRIKE / FACE YOUR SHIELD / CAST");
 text(f,1,3,"A SWORD  HOLD B GUARD  C BOLT");
 if(video_fault || tour_result)text(f,1,3,"CHECK FAILED / RESET DISC");
 f->overlay_kind=world.help?3:world.complete?1:world.game_over?2:0;
 for(u16 i=0;i<160;i++)f->overlay[i]=f->overlay_kind?0x8020:0;
 if(f->overlay_kind){
  for(u16 x=0;x<32;x++)f->overlay[x]=f->overlay[128u+x]=0x802d;
  if(world.help){panel(f,1,2,"A SWING / HOLD B FACE GUARD");panel(f,2,2,"C CAST / START PAUSE");panel(f,3,2,"P2 START JOINS / A BEGIN");}
  else{panel(f,1,7,world.complete?"COURT CLEAR!":"SPARRING ENDED");panel(f,3,8,"B TO TRY AGAIN");}
 }
 for(u16 i=0;i<CO_ACTORS;i++){
  CoActor const *a=&world.actor[i];if(!a->joined || !a->hp)continue;
  u16 tile=i<2?(u16)(ART_HERO+a->face*12u+((a->moving && ((world.tick>>3)&1u))?6u:0)):(u16)(ART_ENEMY+(i-2u)*32u+a->face*8u+((a->moving && ((world.tick>>4)&1u))?4u:0));
  u16 pal=i<2?1:3;
  if(!a->iframes || (world.tick&4u))sprite(f,a->x-8,a->y+12,i<2?16:16,i<2?24:16,tile,pal);
  if(a->guard){s16 x=a->x-16,y=a->y+16;sprite(f,x,y,32,32,(u16)(ART_SHIELD+a->face*16u),1);}
  if(a->state==CO_SWING){
   if(a->age<6)sprite(f,a->x-4,a->y+4,8,8,ART_ALERT,1);
   else if(a->age<13){
    u16 phase=(u16)((a->age-6u)>>1);if(phase>2)phase=2;
    s16 x=a->face==2?a->x-32:a->face==3?a->x:a->x-16;
    s16 y=(s16)(32+a->y+(a->face==1?-32:a->face==0?0:-16));
    sprite(f,x,y,32,32,(u16)(ART_SWORD+a->face*48u+phase*16u),1);
   }
  }
  if(a->flash)sprite(f,a->x-4,a->y+12,8,8,ART_HIT,1);
  if(i>=2){for(u16 n=0;n<a->hp;n++)sprite(f,(s16)(a->x-10+n*6u),a->y+8,8,8,ART_BAR,1);}
 }
 for(u16 i=0;i<CO_BOLTS;i++)if(world.bolt[i].life)sprite(f,world.bolt[i].x-4,world.bolt[i].y+28,8,8,ART_BOLT,1);
 __asm__ volatile("" ::: "memory");published=stage+1u;stage^=1u;
}
static void initialize(void){
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x400)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,ART_COUNT*16u);
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,GROUND_COUNT*16u);
 bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,4);reg(7,2);
 vdp_ctrl_32=CRAM_W;
 for(u16 i=0;i<64;i++){u16 c=(u16)((res_palettes[i*2u]<<8)|res_palettes[i*2u+1u]);bios_palette[i]=c;vdp_data_16=c;}
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=0;
 for(u16 y=0;y<32;y++){
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++)vdp_data_16=0;
 }
 for(u16 y=0;y<24;y++){
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+(y+4u)*128u)|VRAM_W;
  for(u16 x=0;x<40;x++){
   u16 cx=x>>1,cy=y>>1,wall=cx==0 || cx==19 || cy==0 || cy==11 || ((cx==7 || cx==12) && cy==5);
   u16 kind=wall?2u:((cx==1 || cx==18) && (cy==1 || cy==10))?5u:((cx+cy)&1u);
   vdp_data_16=(u16)(0x4000u+GROUND+kind*4u+(y&1u)*2u+(x&1u));
  }
 }
 for(u16 y=0;y<4;y++){
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++)vdp_data_16=0x8020;
 }
}
void main(void){
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 co_init(&world);
#ifdef COMBAT_AUTOTEST
 tour_result=(u16)combat_contract();co_init(&world);
#endif
 initialize();enable_interrupts();combat_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;
 for(;;){
  if(main_ticks==previous)lab_wait();
  u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits,hit2=input_hits2;input_hits=input_hits2=0;enable_interrupts();
  co_advance(&world,delta>=hz?300:(u16)(delta*(hz==50?6u:5u)),bios_joy1_hold,bios_joy2_hold,hit,hit2);
  lab_sound(world.tick,world.events,world.paused);
  combat_publish();
 }
}

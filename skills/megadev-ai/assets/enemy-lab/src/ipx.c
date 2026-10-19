#include "lab.h"
#include "shared.h"
#include "enemy.h"
#include "art_meta.h"
#include "gfx.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
AiWorld world;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
extern int enemy_contract(void);
#define ACTOR 0x80u
#define GROUND 0x200u
#define SPR_CAP 56u
#define HUD 160u
_Static_assert(ACTOR+ART_COUNT<=GROUND && (GROUND+GROUND_COUNT)*32u<BIOS_VDP_DEFAULT_WINDOW,"Art overlaps VDP tables");
typedef struct {GfxList sprites;u16 hud[HUD],overlay[160],overlay_kind;} Frame;
static Frame frames[2];
static u16 stage,hz,shown_overlay;
static volatile u16 published,input_hits,submitted,video_fault,upload_high;
static u16 tour_result;
static void reg(u16 index,u16 value){u16 w=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=w;vdp_ctrl_16=w;}
void lab_line(char const *s,u16 x,u16 y){
 char line[41];u16 n=0;if(x>=40 || y>=4)return;
 while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xff;
 lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u+x*2u)|VRAM_W);
}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
void enemy_input_latch(void){input_hits|=bios_joy1_hit;}
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
static void digit(Frame *f,u16 x,u16 y,u16 value){char s[2]={(char)('0'+value),0};text(f,x,y,s);}
static void icon(Frame *f,u16 x,u16 y,u16 tile){f->hud[y*40u+x]=(u16)(0xa000u+ACTOR+tile);}
static void sprite(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 palette){
 if(y<32 || y>=224 || x<=-(s16)w || x>=320)return;
 gfx_sprite(&f->sprites,x,y,w,h,(u16)(ACTOR+tile),palette,(world.complete || world.game_over)?0:1,0);
}
void enemy_publish(void){
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"ENEMY LAB");
 for(u16 i=0;i<3;i++)icon(f,12u+i*2u,0,i<world.hp?ART_HEART:ART_EMPTY);
 icon(f,21,0,ART_LENS);u16 count=0;for(u16 i=0;i<3;i++)if(world.lenses&(1u<<i))count++;
 digit(f,23,0,count);text(f,24,0,"/3");text(f,34,0,hz==50?"PAL":"NTSC");
 text(f,1,1,"STUN");for(u16 i=0;i<12;i++)icon(f,6+i,1,ART_BAR+(world.cooldown>(11u-i)*15u));
 text(f,21,1,"THE MOONWATCH COURT");
 text(f,1,2,world.paused?"PAUSED":world.complete?"COURTYARD CLEAR! B TRY AGAIN":world.game_over?"CAUGHT! B TRY AGAIN":world.view==1?"SENSE: DOTS SHOW VISIBLE GROUND":world.view==2?"PATH: ROUTES / SHARED 12-NODE BUDGET":"FIND 3 LENSES. EXIT EAST ->");
 text(f,1,3,"A STUN  B RESET  C VIEW  START PAUSE");
 if(video_fault || tour_result)text(f,1,3,"CHECK FAILED / RESET DISC");
 f->overlay_kind=world.complete?1:world.game_over?2:0;
 for(u16 i=0;i<160;i++)f->overlay[i]=f->overlay_kind?0x8020:0;
 if(f->overlay_kind){
  for(u16 x=0;x<32;x++){f->overlay[x]=f->overlay[128u+x]=0x802d;}
  char const *title=world.complete?"COURTYARD CLEAR!":"CAUGHT BY THE WATCH";
  u16 start=world.complete?8:7;
  for(u16 i=0;title[i];i++)f->overlay[32u+start+i]=(u16)(0x8000u|(u8)title[i]);
  char const *again="B TO TRY AGAIN";
  for(u16 i=0;again[i];i++)f->overlay[96u+9u+i]=(u16)(0x8000u|(u8)again[i]);
 }
 if(world.complete){
  sprite(f,world.x-8,(s16)(world.y+12-((world.tick>>3)&3u)),16,24,(u16)(ART_HERO+world.face*12u),1);
  static s16 const sx[8]={24,64,104,144,184,224,264,296},sy[8]={72,176,80,184,72,176,80,184};
  for(u16 i=0;i<8;i++)sprite(f,sx[i],(s16)(sy[i]-((world.tick+i*7u)&15u)),8,8,(u16)(ART_LENS+((world.tick>>3)&1u)),1);
  goto publish;
 }
 /* Actors and state cues are admitted before optional sensing/path markers. */
 if(!world.hurt || (world.tick&4u))sprite(f,world.x-8,world.y+12,16,24,(u16)(ART_HERO+world.face*12u+(world.moving?((world.tick>>3)&1u)*6u:0)),1);
 for(u16 i=0;i<AI_ENEMIES;i++){
  AiEnemy const *e=&world.enemies[i];u16 walk=(e->state!=AI_STUNNED && e->kind!=2)?((world.tick>>4)&1u):0;
  if(e->state!=AI_STUNNED || e->timer>=60 || (world.tick&4u))sprite(f,e->x-8,e->y+20,16,16,(u16)(ART_ENEMY+i*32u+e->face*8u+walk*4u),3);
  s16 top=(s16)(e->y+8-((world.tick>>3)&1u));
  if(e->state==AI_STUNNED)sprite(f,e->x-8,top,16,8,ART_SLEEP,1);
  else if(e->state==AI_ALERT || e->state==AI_CHASE)sprite(f,e->x-4,top,8,8,ART_ALERT,1);
  else if(e->state==AI_SEARCH)sprite(f,e->x-4,top,8,8,ART_QUESTION,1);
 }
 static s16 const lx[3]={40,168,264},ly[3]={40,88,152};
 for(u16 i=0;i<3;i++)if(!(world.lenses&(1u<<i)))sprite(f,lx[i]-4,(s16)(ly[i]+27-((world.tick>>4)&1u)),8,8,(u16)(ART_LENS+((world.tick>>4)&1u)),1);
 for(u16 i=0;i<AI_BOLTS;i++)if(world.bolts[i].life)sprite(f,world.bolts[i].x-4,world.bolts[i].y+28,8,8,ART_BOLT,1);
 if(world.pulse)sprite(f,world.x-16,world.y+16,32,32,(u16)(ART_PULSE+(world.pulse<10?16:0)),1);
 if(world.lenses==7)sprite(f,300,(s16)(64-((world.tick>>3)&1u)),8,8,ART_LENS,1);
 if(world.view==1){
  for(u16 i=0;i<AI_ENEMIES;i++){
   AiEnemy const *e=&world.enemies[i];
   for(u16 n=0;n<8;n++){
    s16 forward=(s16)(24u+(n>>1)*16u),side=(n&1u)?(s16)(forward>>1):(s16)-(forward>>1),x=e->x,y=e->y;
    if(e->face==0){x+=side;y+=forward;}else if(e->face==1){x+=side;y-=forward;}
    else if(e->face==2){x-=forward;y+=side;}else{x+=forward;y+=side;}
    if(ai_sees(&world.map,e,x,y))sprite(f,x-4,y+28,8,8,ART_DOT,1);
   }
  }
 }else if(world.view==2){
  for(u16 i=0;i<AI_ENEMIES;i++){
   AiEnemy const *e=&world.enemies[i];u16 shown=0;
   for(u16 n=e->cursor;n<e->length && shown<8;n++,shown++){
    u16 c=e->route[n];sprite(f,(s16)((c%AI_W)*16u+4u),(s16)((c/AI_W)*16u+36u),8,8,ART_DOT,1);
   }
  }
 }
publish:
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
   u16 cx=x>>1,cy=y>>1,wall=world.map.solid[cy*AI_W+cx];
   u16 kind=wall?2u:cx==1 && cy==10?4u:cx==19 && cy>=2 && cy<=3?4u:((cx==1 || cx==18) && (cy==1 || cy==10))?5u:((cx+cy)&1u);
   if(!wall && cx==11 && cy==8)kind=3;
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
 ai_init(&world);
#ifdef ENEMY_AUTOTEST
 tour_result=(u16)enemy_contract();ai_init(&world);
#endif
 initialize();enable_interrupts();enemy_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;
 for(;;){
  if(main_ticks==previous)lab_wait();
  u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  ai_advance(&world,delta>=hz?300:(u16)(delta*(hz==50?6u:5u)),bios_joy1_hold&15u,hit);
  lab_sound(world.tick,world.events,world.paused);
  enemy_publish();
 }
}

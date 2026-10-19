#include "lab.h"
#include "shared.h"
#include "fx.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
FxWorld world;
extern u8 const res_font[],res_actors[],res_ground[],res_ground_end[],res_map[],res_palettes[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
#define ACTOR 0x80
#define GROUND 0x100
#define SPR_CAP FX_SAT
#define HUD 160
typedef struct {FxView sprites;u16 hud[HUD];} Frame;
_Static_assert(sizeof(Frame)==806,"Publisher ABI changed");
_Static_assert(sizeof(FxWorld)==792,"Effects ABI changed");
static Frame frames[2];
static u16 stage,hz;
static volatile u16 published,input_hits,submitted,video_fault,upload_high;
#ifdef FX_AUTOTEST
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
 vdp_ctrl_32=VSRAM_W;vdp_data_16=(u16)-f->sprites.shake_y;vdp_data_16=(u16)-f->sprites.shake_y;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=(u16)f->sprites.shake_x;vdp_data_16=(u16)f->sprites.shake_x;
 for(u16 y=0;y<4;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128u)|VRAM_W;
  for(u16 x=0;x<40;x++)vdp_data_16=f->hud[y*40u+x];
 }
 u16 bytes=(u16)((count?count:1)*8u+HUD*2u+8u);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s) {
 while(*s && x<40 && y<4){f->hud[y*40+x]=(u16)(0x8000u|(u8)*s);x++;s++;}
}
static void number(Frame *f,u16 x,u16 y,u16 n){char s[6];s[5]=0;for(u16 i=5;i;i--){s[i-1]=(!n && i<5)?' ':(char)('0'+n%10);n/=10;}text(f,x,y,s);}
static u16 frame_rate,last_submitted,window_start,missed;
static u16 rate(u16 frames,u16 blanks){
 u32 scaled=frames;__asm__("mulu.w %1,%0":"+d"(scaled):"d"(hz):"cc");if(!blanks || scaled>65535u)return 0;
 __asm__("divu.w %1,%0":"+d"(scaled):"d"(blanks):"cc");return (u16)scaled;
}
void effects_publish(void) {
 if(published)return;
 Frame *f=&frames[stage];fx_build(&world,&f->sprites);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"EFFECTS LAB");text(f,22,0,"FPS");number(f,26,0,frame_rate);text(f,34,0,hz==50?"PAL":"NTSC");
 text(f,1,1,"A BURST  B WEATHER  C DENSITY");
 text(f,1,2,"D-PAD MOVE  START PAUSE  UP+START SHAKE");
 text(f,1,3,world.paused?"PAUSED":world.weather==0?"CLEAR":world.weather==1?"RAIN":"SNOW");
 text(f,9,3,"DENSITY");char density_text[4]={(char)('1'+world.density),'/','4',0};text(f,17,3,density_text);text(f,22,3,"FX");number(f,24,3,f->sprites.count);
 text(f,30,3,"SKIP");number(f,34,3,f->sprites.omitted);
#ifdef FX_AUTOTEST
 if(tour_ok)text(f,1,0,"AUTO PASS  ");
#endif
 if(video_fault)text(f,1,0,"VIDEO FAULT");
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize(void) {
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actors,to_vdp_addr(ACTOR*32u)|VRAM_W,13*16);
 lab_upload_work(res_ground,to_vdp_addr(GROUND*32u)|VRAM_W,(u32)(res_ground_end-res_ground)/2u);
 bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,4);reg(7,33);
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
 for(u16 y=0;y<32;y++) {
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++){u16 i=(u16)(((y%28)*40+x%40)*2);vdp_data_16=(u16)((res_map[i]<<8)|res_map[i+1]);}
 }
}
#ifdef FX_AUTOTEST
void effects_tour(void){
 FxWorld test;FxView v;fx_init(&test);
 for(u16 i=0;i<1200;i++){
  fx_advance(&test,hz==50?6:5,8,(i%12)==0?64:0);fx_build(&test,&v);
  if(v.count>FX_SAT)return;
  for(u16 b=0;b<28;b++)if(v.bands[b]>16)return;
 }
 tour_ok=1;
}
#endif
void main(void) {
 disable_interrupts();system_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 fx_init(&world);initialize();enable_interrupts();effects_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;window_start=previous;
#ifdef FX_AUTOTEST
 effects_tour();
#endif
 for(;;){
  lab_wait();u16 now=main_ticks,delta=(u16)(now-previous);previous=now;
  if(delta>1)missed+=(u16)(delta-1);
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
#ifdef FX_AUTOTEST
  if(!(world.tick%12))hit|=64;
#endif
  fx_advance(&world,delta>4?20:(u16)(delta*(hz==50?6:5)),bios_joy1_hold&15u,hit);
  lab_sound(world.tick,hit&64,world.paused);effects_publish();
  if((u16)(now-window_start)>=hz){frame_rate=rate((u16)(submitted-last_submitted),(u16)(now-window_start));last_submitted=submitted;window_start=now;}
 }
}

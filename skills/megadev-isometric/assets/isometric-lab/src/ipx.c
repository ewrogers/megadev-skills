#include "lab.h"
#include "shared.h"
#include "controller.h"
#include "packed.h"
#include <main/memmap.h>
#include "art_meta.h"
#include "gfx.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
IsoWorld world;
extern u8 const res_font[],res_actors[],res_ground[],res_palettes[],res_map[],res_actors_end[],res_ground_end[],res_map_end[];
extern void system_install(void);
extern void lab_sound(u16 tick,u16 events,u16 paused);
extern int isometric_contract(void);
#define ACTOR 0x80u
#define GROUND 0x280u
#define SPR_CAP 56u
#define HUD 160u
_Static_assert(ACTOR+ART_COUNT<=GROUND && (GROUND+ART_GROUND_COUNT)*32u<BIOS_VDP_DEFAULT_WINDOW,"Art overlaps VDP tables");
typedef struct {GfxList sprites;u16 hud[HUD],scroll_x,scroll_y;} Frame;
_Static_assert(sizeof(Frame)==1644,"Frame ABI changed");
_Static_assert(ART_COUNT*32u<=16384 && ART_GROUND_COUNT*32u<=16384,"Startup scratch overflow");
static Frame frames[2];
static u16 stage,hz;
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
void isometric_input_latch(void){input_hits|=bios_joy1_hit;}
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
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=f->scroll_x;
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=f->scroll_y;
 u16 bytes=(u16)((count?count:1)*8u+HUD*2u+8u);if(bytes>upload_high)upload_high=bytes;
 submitted++;published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s){while(*s && x<40 && y<4)f->hud[y*40u+x++]=(u16)(0x8000u|(u8)*s++);}
static void number(Frame *f,u16 x,u16 y,u16 n){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(n>>(12-i*4))&15];s[4]=0;text(f,x,y,s);}
static void draw(Frame *f,s16 x,s16 y,u16 w,u16 h,u16 tile,u16 palette){
 for(u16 row=0;row<h;row+=32){
  u16 part=h-row>32?32:h-row;
  gfx_sprite(&f->sprites,x,(s16)(y+row),w,part,(u16)(ACTOR+tile),palette,0,0);tile+=(w>>3)*(part>>3);
 }
}
void isometric_publish(void){
 if(published)return;
 Frame *f=&frames[stage];gfx_begin(&f->sprites,SPR_CAP);
 for(u16 i=0;i<HUD;i++)f->hud[i]=0x8020;
 text(f,1,0,"LANTERN CONSERVATORY");text(f,34,0,hz==50?"PAL":"NTSC");
 text(f,1,1,world.paused?"PAUSED":world.game.layer?"UPPER WALK / STAIRS AT BOTH ENDS":"GARDEN WALK / PASS UNDER THE BRIDGE");
 text(f,1,2,"A TOUR  B CUTAWAY  C INFO");
 if(world.info){text(f,1,2,"HEIGHT");number(f,8,2,(u16)(world.game.z>>4));text(f,15,2,"ORDER");}
 text(f,1,3,"D-PAD WALK  START PAUSE");
 if(world.cutaway)text(f,27,3,"CUTAWAY ON");
 u16 count=iso_order(&world);
 if(world.info)text(f,22,2,world.sort_status==ISO_SORT_OK?"OK":world.sort_status==ISO_SORT_AMBIGUOUS?"AMBIGUOUS":"CHECK FAILED");
 if(video_fault || tour_result || world.sort_status==ISO_SORT_INVALID || world.sort_status==ISO_SORT_CYCLE)text(f,1,3,"ORDER / VIDEO CHECK FAILED");
 static u16 const tiles[8]={ART_DECK,ART_POST,ART_RAMP_L,ART_RAMP_R,ART_TOWER,ART_TREE,ART_FOUNTAIN,ART_HERO};
 static u16 const widths[8]={32,16,32,32,32,32,32,24},heights[8]={24,40,48,48,64,48,32,32};
 static s16 const ax[8]={16,8,16,16,16,16,16,12},ay[8]={12,35,36,36,55,40,24,29};
 if(world.info){IsoPoint at=iso_project(world.game.x,world.game.y,world.game.z);draw(f,(s16)(248+at.x-world.game.camera_x),(s16)(60+at.y-world.game.camera_y),16,8,ART_MARKER,3);}
 /* Sort gives back-to-front; earlier hardware SAT entries win overlap. */
 if(world.sort_status!=ISO_SORT_INVALID)for(u16 rank=count;rank;rank--){
  u16 i=world.order[rank-1u],hero=i==ISO_STATIC,kind=hero?ISO_HERO:iso_props[i].kind;
  if(world.cutaway && !hero && (kind==ISO_DECK || kind==ISO_RAMP_L || kind==ISO_RAMP_R))continue;
  s16 x=hero?world.game.x:iso_props[i].x,y=hero?world.game.y:iso_props[i].y,z=hero?world.game.z:iso_props[i].z;
  IsoPoint at=iso_project(x,y,z);u16 tile=tiles[kind];
  if(hero)tile+=(world.game.facing*4u+(world.game.moving?world.game.walk>>3:0))*12u;
  if(kind==ISO_FOUNTAIN)tile+=((world.game.ticks>>4)%3u)*16u;
  draw(f,(s16)(256+at.x-world.game.camera_x-ax[kind]),(s16)(64+at.y-world.game.camera_y-ay[kind]),widths[kind],heights[kind],tile,hero?3:2);
 }
 f->scroll_x=(u16)-world.game.camera_x;f->scroll_y=(u16)world.game.camera_y;
 __asm__ volatile("" ::: "memory");published=stage+1u;stage^=1u;
}
void isometric_initialize(void){
 lab_display_off();lab_clear();
 lab_upload_work(res_font,to_vdp_addr(0x400)|VRAM_W,96*16);
 /* Boot has finished relocating Main; Sub granted 2M Word RAM and never
    touches it again in this Lab. Reuse only the first 16 KiB as startup scratch. */
 u8 *scratch=(u8 *)WORD_RAM;
 if(!iso_unpack(res_actors,(u16)(res_actors_end-res_actors),scratch,ART_COUNT*32u)){video_fault=1;return;}
 lab_upload_word(scratch,to_vdp_addr(ACTOR*32u)|VRAM_W,ART_COUNT*16u);
 if(!iso_unpack(res_ground,(u16)(res_ground_end-res_ground),scratch,ART_GROUND_COUNT*32u)){video_fault=1;return;}
 lab_upload_word(scratch,to_vdp_addr(GROUND*32u)|VRAM_W,ART_GROUND_COUNT*16u);
 if(!iso_unpack(res_map,(u16)(res_map_end-res_map),scratch,4096)){video_fault=1;return;}
 bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,4);reg(7,2);
 vdp_ctrl_32=CRAM_W;
 for(u16 i=0;i<64;i++){u16 c=(u16)((res_palettes[i*2u]<<8)|res_palettes[i*2u+1u]);bios_palette[i]=c;vdp_data_16=c;}
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=0;
 for(u16 y=0;y<32;y++){
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++)vdp_data_16=0;
 }
 for(u16 y=0;y<32;y++){
  vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++){u16 i=(y*64u+x)*2u;vdp_data_16=(u16)(0x2000u+GROUND+((scratch[i]<<8)|scratch[i+1u]));}
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
 iso_world_init(&world);
#ifdef ISOMETRIC_AUTOTEST
 tour_result=(u16)isometric_contract();iso_world_init(&world);
#endif
 isometric_initialize();enable_interrupts();isometric_publish();
 do{lab_wait();}while(published);lab_display_on();u16 previous=main_ticks;
 for(;;){
  if(main_ticks==previous)lab_wait();
  u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  iso_advance(&world,delta>=hz?300:(u16)(delta*(hz==50?6u:5u)),bios_joy1_hold,hit);
  lab_sound(world.game.ticks,world.events,world.paused);
  isometric_publish();
 }
}

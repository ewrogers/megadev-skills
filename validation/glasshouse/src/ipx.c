#include "lab.h"
#include "shared.h"
#include "stream.h"
#include "audio.h"
#include "scene.h"
#include "checkpoint.h"
#include "contracts.h"
#include "garden.h"
#include "paint.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_font[],res_actor[],res_ui[],res_palette[];
extern void stream_install(void);
#define ACTOR_TILE 0x80
#define SLOT0 0xC0
#define SLOT1 0x340
#define UP_WORDS 160
_Static_assert((SLOT1+STREAM_TILES)*32u==BIOS_VDP_DEFAULT_SPRLIST,"Pattern slots overlap VDP tables");
_Static_assert(0xB0+9<=SLOT0 && ACTOR_TILE+48==0xB0,"Resident art overlap");
typedef struct {UiBatch ui;u16 palette[64],sat[8],data[UP_WORDS],address,words,row,has_ui,palette_mask;} Frame;
static Frame frames[2];
static UiPainter painter;
static UiCanvas canvas;
Garden garden;
Scene flow;
static u16 maps[2][STREAM_MAP],palettes[64],displayed_palette[64],cached[2];
static u16 stage,active=2,target,loaded_slot,hz,transition,map_row,fade=8;
static u16 errors,previous_pass,booted,load_started,preload_generation,inject_missing,commit_prepared;
static u16 music_wanted=2,music_sent;
static u16 checkpoint_lease,checkpoint_generation,checkpoint_origin,checkpoint_op,restore_pending,restore_intent;
static SaveGame restored;
static volatile u16 published,completed,input_hits,upload_ack,video_fault,upload_high;
#ifdef GLASS_AUTOTEST
static u16 tour_step,tour_ok,due;
#endif
static u16 tile_base(u16 slot){return slot?SLOT1:SLOT0;}
static void reg(u16 index,u16 value){u16 w=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=w;vdp_ctrl_16=w;}
void lab_line(char const *s,u16 x,u16 y) {
 char line[41];u16 n=0;if(x>=40 || y>=28)return;
 while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
 lab_print(line,to_vdp_addr(UI_PAGE0+y*128u+x*2u)|VRAM_W);
}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
/* Validation is complete before the first MMIO write. Late blank keeps the
   immutable publication pending; it never partly applies a palette or page. */
void vblank_user(void) {
 main_ticks++;input_hits|=bios_joy1_hit;
 u16 index=published;if(!index)return;
 if(index>2){video_fault=1;published=0;return;}
 Frame const *f=&frames[index-1];
 if(f->words>UP_WORDS || (f->row && f->words!=160) || f->has_ui>1 || (f->has_ui && !ui_batch_valid(&f->ui))){video_fault=1;published=0;return;}
 u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232)return;
 u16 bytes=16;
 for(u16 line=0;line<4;line++)if(f->palette_mask&(1u<<line)) {
  vdp_ctrl_32=to_vdp_addr(line*32u)|CRAM_W;
  for(u16 i=0;i<16;i++){u16 at=line*16u+i;vdp_data_16=f->palette[at];bios_palette[at]=f->palette[at];displayed_palette[at]=f->palette[at];}bytes+=32;
 }
 if(f->has_ui) {
  u16 base=f->ui.page?UI_PAGE1:UI_PAGE0;
  for(u16 i=0;i<f->ui.count;i++) {
   vdp_ctrl_32=to_vdp_addr(base+f->ui.rows[i]*128u)|VRAM_W;
   for(u16 x=0;x<40;x++)vdp_data_16=f->ui.words[i][x];
  }bytes+=f->ui.count*80u;
  if(f->ui.flip)reg(2,(u16)(base>>10));
 }
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
 for(u16 i=0;i<8;i++)vdp_data_16=f->sat[i];
 if(f->words) {
  if(f->row) {
   for(u16 r=0;r<4;r++) {
    vdp_ctrl_32=to_vdp_addr(f->address+r*128u)|VRAM_W;
    for(u16 x=0;x<40;x++)vdp_data_16=f->data[r*40u+x];
   }
  } else {
   vdp_ctrl_32=to_vdp_addr(f->address)|VRAM_W;
   for(u16 i=0;i<f->words;i++)vdp_data_16=f->data[i];
   upload_ack=f->words*2u;
  }bytes+=f->words*2u;
 }
 if(bytes>upload_high)upload_high=bytes;
 completed=index;published=0;
}
static u16 shade(u16 c,u16 amount) {
 u16 result=0;
 for(u16 shift=0;shift<12;shift+=4) {
  u32 product=(c>>shift)&14u;__asm__("mulu.w %1,%0":"+d"(product):"d"(amount):"cc");
  result|=(u16)((product>>3)&14u)<<shift;
 }return result;
}
void stream_prepare(void) {
 if(published || completed)return;
 Frame *f=&frames[stage];f->words=f->row=0;
 garden_draw(&canvas,&garden,hz,stream_phase,checkpoint_pending?14:checkpoint_result);
#ifdef GLASS_AUTOTEST
 if(tour_ok)ui_text(&canvas,2,3,34,0,0,"AUTO PASS - INTEGRATED RECOVERY");
#endif
 if(errors || video_fault || audio_link_fault)ui_text(&canvas,2,3,34,0,0,"SERVICE FAULT - RESET REQUIRED");
 f->has_ui=(u16)ui_prepare(&painter,&canvas,garden_screen(&garden),&f->ui);
 for(u16 i=0;i<64;i++)f->palette[i]=i>=16?shade(palettes[i],fade):palettes[i];
 f->palette_mask=0;for(u16 i=0;i<64;i++)if(f->palette[i]!=displayed_palette[i])f->palette_mask|=(u16)(1u<<(i>>4));
 u16 visible=booted && scene_world(flow.current) && flow.phase==SC_RUN && garden.mode<=G_TALK;
 u16 frame=(u16)(garden.face*2u+(garden.walking?((garden.tick>>3)&1u):0u));
 f->sat[0]=visible?(u16)(128+garden.y-22):0;f->sat[1]=0x0601;
 f->sat[2]=(u16)(0xA000|(ACTOR_TILE+frame*6u));f->sat[3]=(u16)(128+garden.x-8);
 f->sat[4]=visible && !garden.area?128+64-22:0;f->sat[5]=0x0600;f->sat[6]=0xA000|(ACTOR_TILE+12);f->sat[7]=128+64-8;
 if(transition==2) {
  f->row=1;f->address=(u16)(BIOS_VDP_DEFAULT_PLANEB+(4+map_row)*128u);f->words=160;
  for(u16 i=0;i<160;i++){u16 cell=maps[target][map_row*40u+i];f->data[i]=(u16)((cell&0x1800u)|((cell&0x7FFu)+tile_base(target))|((target+2u)<<13));}
  map_row+=4;if(map_row==20){active=target;transition=3;}
 } else if(stream_phase==ST_UPLOAD) {
  if((*ga_reg_memmode&7u)!=1u){errors++;stream_cancel();}
  else {
   u16 remaining=(u16)(stream_cache_tiles*32u-stream_uploaded),count=STREAM_CHUNK;
   if(remaining<count)count=remaining;
   f->words=count/2;f->address=(u16)(tile_base(loaded_slot)*32u+stream_uploaded);
   volatile unsigned char *p=(volatile unsigned char *)WORD_RAM+64+stream_uploaded;
   for(u16 i=0;i<f->words;i++)f->data[i]=stream_word(p+i*2u);
  }
 }
 disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
void garden_ack(void) {
 u16 done=completed;if(!done)return;
 if(frames[done-1].has_ui && !ui_ack(&painter,&frames[done-1].ui))errors++;
 completed=0;
 garden.ready=painter.screen==garden_screen(&garden) && !painter.building;
}
int relay_request(u16 which) {
 if(!checkpoint_pending && stream_start(which)){loaded_slot=which;if(which<2)cached[which]=0;return 1;}return 0;
}
static int area_is_black(void) {
 if(active>1)return 1;
 for(u16 i=0;i<16;i++)if(displayed_palette[(active+2u)*16u+i])return 0;
 return 1;
}
void garden_goto(u16 scene) {
 if(scene_request(&flow,scene)){commit_prepared=0;if(stream_phase)stream_cancel();}
}
void scene_service(u16 elapsed) {
 u16 events=0;if(area_is_black())events|=SC_BLACK;
 if(!checkpoint_pending && !stream_phase && !*ga_reg_comcmd0 && !*ga_reg_comstat0 && (*ga_reg_memmode&7u)==1u)events|=SC_RETIRED;
 if(preload_generation && events&SC_RETIRED){scene_release(&flow,preload_generation);preload_generation=0;}
 if(flow.phase==SC_DRAIN && stream_phase)stream_cancel();
 u16 asset=flow.target==SC_CAVERN?1:0;
 if(flow.phase==SC_LOAD) {
  if(!load_started){if(relay_request(inject_missing?2:asset))load_started=1;}
  else if(events&SC_RETIRED)events|=cached[asset] && !inject_missing?SC_READY:SC_FAILED;
 }
 if(flow.phase==SC_COMMIT && !flow.leases && !published && !completed) {
  if(!commit_prepared) {
   if(scene_world(flow.target)) {
    if(flow.target!=flow.current || garden.mode==G_TITLE){garden_enter(&garden,asset);if(flow.current==SC_TITLE){garden.x=48;garden.y=128;}}
    if(restore_pending && flow.target==restored.level){garden.x=restored.coins;garden.y=restored.flags>>4;garden.progress=restored.flags&15u;}
   } else {garden.mode=G_TITLE;garden.ready=0;}
   restore_pending=0;commit_prepared=1;
  }
  if(scene_world(flow.target) && active!=asset && !transition){target=asset;map_row=0;transition=2;}
  if((!scene_world(flow.target) || active==asset) && (!transition || transition==3) && painter.screen==garden_screen(&garden) && !painter.building){transition=0;events|=SC_PUBLISHED;}
 }
 u16 old=flow.phase;scene_tick(&flow,elapsed,events,inject_missing?0:cached[asset]);
 if(old==SC_IN && flow.phase==SC_OUT)commit_prepared=0;
 if(old==SC_COMMIT && flow.phase==SC_IN){load_started=0;inject_missing=0;music_wanted=flow.current==SC_CAVERN?3:2;}
 fade=flow.fade;
}
int checkpoint_begin(Sv16 op) {
 if(flow.phase!=SC_RUN || (op==CP_LOAD?flow.current!=SC_TITLE || garden.mode!=G_TITLE:op!=CP_SAVE || garden.mode!=G_PAUSE))return 0;
 SaveGame snapshot={flow.current,garden.x,(Sv16)((garden.y<<4)|garden.progress)};
 u16 lease=0;
 if(op==CP_SAVE){if(!scene_hold(&flow,flow.generation))return 0;lease=flow.generation;}
 if(!checkpoint_request(op,op==CP_SAVE?&snapshot:0)){if(lease)scene_release(&flow,lease);garden.notice=1;return 0;}
 garden.notice=0;
 checkpoint_lease=lease;checkpoint_generation=flow.generation;checkpoint_origin=flow.current;checkpoint_op=op;restore_intent=op==CP_LOAD;return 1;
}
void checkpoint_service(Sv16 elapsed) {
 u16 previous=checkpoint_pending;checkpoint_main_tick(elapsed);
 if(restore_intent && (garden.mode!=G_TITLE || garden.menu!=1 || flow.phase!=SC_RUN))restore_intent=0;
 if(!previous || checkpoint_pending)return;
 if(checkpoint_lease){scene_release(&flow,checkpoint_lease);checkpoint_lease=0;}
 if(checkpoint_op==CP_LOAD && restore_intent && (checkpoint_result==SV_LOADED || checkpoint_result==SV_RECOVERED) &&
    checkpoint_generation==flow.generation && checkpoint_origin==flow.current && flow.current==SC_TITLE && flow.phase==SC_RUN) {
  restored=checkpoint_reply;
  if(bp_checkpoint(restored.level,restored.coins,restored.flags)){garden_goto(restored.level);restore_pending=1;}
 }restore_intent=0;
}
static void initialize(void) {
 lab_display_off();lab_clear();ui_painter_init(&painter);
 lab_upload_work(res_font,to_vdp_addr(0x20*32u)|VRAM_W,96*16);
 lab_upload_work(res_actor+64,to_vdp_addr(ACTOR_TILE*32u)|VRAM_W,48*16);
 lab_upload_work(res_ui,to_vdp_addr(0xB0*32u)|VRAM_W,9*16);
 for(u16 i=0;i<64;i++)palettes[i]=i<16?stream_word(res_palette+i*2u):i<32?stream_word(res_actor+32+(i-16)*2u):0;
 bios_vdp_update_flags=0;reg(11,0);reg(16,1);reg(17,0);reg(18,0);reg(7,2);reg(2,UI_PAGE0>>10);
 vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++){vdp_data_16=palettes[i];bios_palette[i]=palettes[i];displayed_palette[i]=palettes[i];}
 for(u16 page=0;page<2;page++)for(u16 y=0;y<32;y++) {
  vdp_ctrl_32=to_vdp_addr((page?UI_PAGE1:UI_PAGE0)+y*128u)|VRAM_W;
  for(u16 x=0;x<64;x++)vdp_data_16=0x8020;
 }
 vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=0;
 vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
 lab_display_on();
}
#ifdef GLASS_AUTOTEST
/* Disc/audio/scene integration tour never writes save media. */
void glass_tour(void) {
 if(!booted || flow.phase!=SC_RUN || !garden.ready || (u16)(main_ticks-due)>=0x8000u)return;
 if(tour_step==0){garden.progress=1;garden_goto(SC_QUAY);tour_step++;}
 else if(tour_step==1){if(scene_hold(&flow,flow.generation)){preload_generation=flow.generation;relay_request(1);}garden_goto(SC_TITLE);tour_step++;}
 else if(tour_step==2){garden_goto(SC_CAVERN);tour_step++;}
 else if(tour_step==3){garden.progress=7;garden_goto(SC_QUAY);tour_step++;}
 else if(tour_step==4){garden_goto(SC_TITLE);tour_step++;}
 else if(tour_step==5){inject_missing=1;cached[0]=0;garden_goto(SC_QUAY);tour_step++;}
 else if(flow.current==SC_TITLE && flow.failures==1 && !flow.leases && !errors && !video_fault && !audio_link_fault && !checkpoint_pending)tour_ok=1;
 due=(u16)(main_ticks+hz);
}
#endif
void main(void) {
 disable_interrupts();stream_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 garden_init(&garden);scene_init(&flow);flow.fade=fade=0;initialize();enable_interrupts();relay_request(0);
 u16 previous=main_ticks;
 for(;;) {
  lab_wait();u16 now=main_ticks,delta=(u16)(now-previous),elapsed=0;previous=now;
  if(!bp_elapsed(now,(u16)(now-delta),hz,&elapsed))errors++;
  disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
  garden_ack();audio_main_tick(elapsed);
  if(upload_ack){u16 bytes=upload_ack;upload_ack=0;stream_upload_done(bytes);}
  stream_main_tick(elapsed);
  if(stream_pass!=previous_pass) {
   previous_pass=stream_pass;cached[loaded_slot]=1;
   for(u16 i=0;i<STREAM_MAP;i++)maps[loaded_slot][i]=stream_cache_map[i];
   for(u16 i=0;i<16;i++)palettes[(loaded_slot+2u)*16u+i]=stream_cache_palette[i];
   if(!booted){target=loaded_slot;transition=2;map_row=0;}
  }
  if(!booted && transition==3 && !published && !completed){transition=0;booted=1;flow.phase=SC_IN;}
  checkpoint_service(elapsed);if(booted)scene_service(elapsed);
  if(music_sent!=music_wanted && !audio_pending && audio_request(AU_PLAY,music_wanted))music_sent=music_wanted;
#ifdef GLASS_AUTOTEST
  hit=0;glass_tour();
#endif
  if(booted && flow.phase==SC_RUN) {
   u16 request=garden_step(&garden,elapsed,bios_joy1_hold&15u,hit);
   flow.paused=garden.mode!=G_FIELD;
   if(request&G_EFFECT){if(!audio_pending)audio_request(AU_EFFECT,0);if(garden.progress&G_BLOOM)music_wanted=3;}
   if(request&G_TRAVEL)garden_goto(flow.current==SC_TITLE?SC_QUAY:flow.current==SC_QUAY?SC_CAVERN:SC_QUAY);
   if(request&G_SAVE)checkpoint_begin(CP_SAVE);
   if(request&G_RESUME)checkpoint_begin(CP_LOAD);
   if(request&G_LEAVE)garden_goto(SC_TITLE);
   if(garden.mode==G_FIELD && !checkpoint_pending && !stream_phase && !cached[garden.area?0:1] && ((garden.area && garden.x<112) || (!garden.area && garden.x>192))) {
    if(scene_hold(&flow,flow.generation)) {
     preload_generation=flow.generation;
     if(!relay_request(garden.area?0:1)){scene_release(&flow,preload_generation);preload_generation=0;}
    }
   }
  }
  stream_prepare();
 }
}

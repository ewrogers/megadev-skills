#include "lab.h"
#include "shared.h"
#include "cinema.h"
#include <main/bios.h>
#include <main/io.def.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
#define CI_PLANEB 0xa000u
_Static_assert((CI_SLOT0+240u)<=CI_SLOT1 && (CI_SLOT1+240u)*32u<0x9000u,"Frame slots must not overlap each other or tables");
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_font[],res_palette[],res_poster[];
extern void stream_install(void);
extern V16 cinema_copy_checked(volatile void const *source,void *destination);
_Static_assert(CI_BYTES==7680u,"Update the fixed copy kernel when changing frame geometry");
void cinema_hud_vblank(void);
Cinema cinema;
/* Native VDP frame plus a copied, immutable interrupt batch: no queued Word RAM pointers. */
static V16 image[CI_BYTES/2];
typedef struct {V16 words,address,kind,slot,data[CI_BATCH/2];} VideoBatch;
static VideoBatch batch;
static volatile V16 published,ack,hits,video_fault;
static V16 prefetched;
static V16 loaded,playing,stopping,failed,slow,hz,link,token=2,requested,age,result;
static V16 rate_clock,rate_count,images_per_second;
volatile V16 failure_site;
static V16 displayed[200],hud_dirty;
static V16 slot_base(V16 slot){return slot?CI_SLOT1:CI_SLOT0;}
static void reg(V16 n,V16 value){V16 w=(V16)(0x8000u|(n<<8)|value);bios_vdp_regs[n]=w;vdp_ctrl_16=w;}
void lab_line(char const *s,u16 x,u16 y){char line[41];u16 n=0;while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xff;lab_print(line,to_vdp_addr(CI_PLANEB+y*128u+x*2u)|VRAM_W);}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
void cinema_input_latch(void){hits|=bios_joy1_hit;}
void vblank_user(void){
 main_ticks++;*ga_reg_comcmd3=main_ticks;

 V16 vertical=(V16)(*(volatile V16 *)0xc00008u>>8);
 if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=232){return;}
 cinema_hud_vblank();
 if(!published)return;
 if(batch.words>CI_BATCH/2 || batch.slot>1 || batch.slot==cinema.slot || batch.kind<1 || batch.kind>2 ||
   (batch.kind==1 && (!batch.words || batch.address<slot_base(batch.slot)*32u ||
    batch.address+batch.words*2u>(slot_base(batch.slot)+240u)*32u)) ||
   (batch.kind==2 && (batch.words || cinema.uploaded!=CI_BYTES))) {video_fault=1;published=0;return;}
 if(batch.kind==1)lab_upload_work(batch.data,to_vdp_addr(batch.address)|VRAM_W,batch.words);
 else reg(2,batch.slot?0x38:0x30); /* Complete hidden page only, reflected in BIOS cache. */
 ack=batch.kind;__asm__ volatile("" ::: "memory");published=0;
}
void cinema_request(V16 frame){
 if(link || *ga_reg_comcmd0 || *ga_reg_comstat0 || (*ga_reg_memmode&7u)!=1u)return;
 token++;if(token<2)token=2;requested=frame;age=0;*ga_reg_comcmd1=frame;
 __asm__ volatile("" ::: "memory");*(volatile V8 *)(GA_REG_MEMMODE+1)|=2u;link=1;
}
static __attribute__((noinline)) void stop_at(V16 error,V16 site){if(!failure_site)failure_site=site;stopping=1;playing=0;failed=error;*ga_reg_comcmd4=0;}
void cinema_receive(V16 elapsed){
 if(!link)return;
 if(age<CI_TIMEOUT){age=elapsed>=CI_TIMEOUT-age?CI_TIMEOUT:(V16)(age+elapsed);if(age==CI_TIMEOUT)stop_at(CI_TIMEOUT_ERROR,1);}
 V16 reply=*ga_reg_comstat0,mode=*ga_reg_memmode;
 if(link==1){if((mode&7u)==2u){__asm__ volatile("" ::: "memory");*ga_reg_comcmd0=token;link=2;}return;}
 if(link==2){
  if(reply!=token)return;
  if((mode&7u)!=1u || *ga_reg_comstat6!=token || *ga_reg_comstat2!=requested){stop_at(CI_FORMAT,3);return;}
  result=*ga_reg_comstat1;
  if(result!=CI_OK)stop_at(result,2);
  if(!stopping && loaded){
   V16 last_copied=cinema.uploaded==CI_BYTES ||
    ((published || ack==1) && batch.kind==1 && cinema.uploaded+batch.words*2u==CI_BYTES);
   if(!last_copied || prefetched)return;
  } /* The final DMA packet owns its copy; image[] may receive the next frame. */
  if(!stopping){
   V16 sum=cinema_copy_checked((volatile void const *)WORD_RAM,image);
   if(sum!=*ga_reg_comstat5)stop_at(CI_FORMAT,4);
   else if(loaded)prefetched=requested+1u;
   else if(!ci_begin(&cinema,requested))stop_at(CI_FORMAT,5);
   else loaded=1;
  }
  __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=0;link=3;return;
 }
 if(link==3 && !reply){if((mode&7u)!=1u){stop_at(CI_FORMAT,6);return;}link=0;}
}
static void text(V16 *row,V16 x,char const *s){while(*s && x<40)row[x++]=(V16)(0x8000u|(V8)*s++);}
static void number(V16 *row,V16 x,V16 n){char s[6];s[5]=0;for(V16 i=5;i;i--){s[i-1]=(char)('0'+n%10u);n/=10u;}text(row,x,s);}
static void hud(void){
 /* A small fixed number of changed glyphs, inside early VBlank via the same owner. */
 V16 rows[200];for(V16 i=0;i<200;i++)rows[i]=0x8020;
 text(rows,1,"CINEMA LAB");text(rows,30,hz==50?"PAL":"NTSC");
 text(rows+40,1,failed?"READ ERROR - C DRAIN":stopping?(link || *ga_reg_comstat7)?"DRAINING CD READ":"RETURNED SAFELY":!playing?cinema.done?"THE END - A REPLAY":"A PLAY LANTERN FLIGHT":cinema.paused?"PAUSED":loaded?"LANTERN FLIGHT":"BUFFERING - HOLD LAST IMAGE");
 if(failed){for(V16 x=0;x<40;x++)rows[40+x]=0x8020;text(rows+40,1,"READ ERROR");number(rows+40,12,failed);text(rows+40,19,"AT");number(rows+40,22,failure_site);text(rows+40,29,"C DRAIN");}
 text(rows+80,1,"IMAGE");number(rows+80,7,cinema.presented);text(rows+80,14,"OF 48");
 text(rows+80,22,slow?"256B":"4096B");text(rows+80,29,"LATE");number(rows+80,34,cinema.late);
 text(rows+120,1,"IMAGES/SEC");number(rows+120,12,images_per_second);text(rows+120,21,"TARGET 10");
 text(rows+160,1,"A PLAY  B UPLOAD  C SKIP  START HOLD");
 /* Called by foreground only when publication is idle. Pending changed cells
    are committed by the VBlank callback below, never while active. */
 for(V16 i=0;i<200;i++)if(displayed[i]!=rows[i]){displayed[i]=rows[i];hud_dirty=1;}
}
/* Keep HUD writes under one row per blank and reserve image DMA bandwidth. */
static V16 hud_row;
void cinema_hud_vblank(void){
 if(!hud_dirty)return;
 V16 y=hud_row==0?1:hud_row==1?3:hud_row==2?22:hud_row==3?24:26;
 vdp_ctrl_32=to_vdp_addr(CI_PLANEB+y*128u)|VRAM_W;
 for(V16 x=0;x<40;x++)vdp_data_16=displayed[hud_row*40u+x];
 hud_row++;if(hud_row==5){hud_row=0;hud_dirty=0;}
}
void cinema_initialize(void){
 lab_display_off();lab_clear();lab_upload_work(res_font,to_vdp_addr(0x400)|VRAM_W,96*16);
 lab_upload_work(res_poster,to_vdp_addr(CI_SLOT0*32u)|VRAM_W,CI_BYTES/2u);
 reg(4,CI_PLANEB>>13);reg(3,0x9000>>10);reg(11,0);reg(17,0);reg(18,0);reg(7,2);reg(2,0x30);bios_vdp_update_flags=0;
 vdp_ctrl_32=CRAM_W;
 for(V16 i=0;i<64;i++){V16 c=i==1?0x0eee:i==2?0x0200:i>=16 && i<32?ci_word(res_palette+(i-16u)*2u):0;bios_palette[i]=c;vdp_data_16=c;}
 for(V16 slot=0;slot<2;slot++)for(V16 y=0;y<28;y++){
  vdp_ctrl_32=to_vdp_addr((slot?0xe000u:0xc000u)+y*128u)|VRAM_W;
  for(V16 x=0;x<40;x++)vdp_data_16=y>=8 && y<20 && x>=10 && x<30?(V16)(0xa000u+slot_base(slot)+(y-8u)*20u+x-10u):0;
 }
 /* Fixed navy letterbox and native font, no palette changes during playback. */
 for(V16 y=0;y<28;y++){vdp_ctrl_32=to_vdp_addr(CI_PLANEB+y*128u)|VRAM_W;for(V16 x=0;x<40;x++)vdp_data_16=0x8020;}
 ci_init(&cinema);hud();lab_display_on();
}
void cinema_acknowledge(V16 done){
  if(done==1){if(!ci_upload(&cinema,batch.words*2u))stop_at(CI_FORMAT,7);}
  if(done==2){rate_count++;V16 pause=cinema.paused;cinema.paused=0;if(!ci_present(&cinema))stop_at(CI_FORMAT,8);cinema.paused=pause;loaded=0;
   if(prefetched && !stopping){if(!ci_begin(&cinema,prefetched-1u))stop_at(CI_FORMAT,9);else loaded=1;prefetched=0;}
   *ga_reg_comcmd5=cinema.presented;if(cinema.done){playing=0;*ga_reg_comcmd4=0;}}
}
/* An IRQ may acknowledge a flip during a foreground frame copy. Retire that
   acknowledgement before constructing another packet from the old state. */
void cinema_publish(void){
  if(playing && !stopping && !cinema.paused && !published && !ack){
   if(loaded){
    if(cinema.uploaded<CI_BYTES){
     V16 count=slow?256:CI_BATCH;if(count>CI_BYTES-cinema.uploaded)count=CI_BYTES-cinema.uploaded;
     batch.kind=1;batch.slot=cinema.slot^1u;batch.address=(V16)(slot_base(batch.slot)*32u+cinema.uploaded);batch.words=count/2u;
     for(V16 i=0;i<batch.words;i++)batch.data[i]=image[cinema.uploaded/2u+i];
     __asm__ volatile("" ::: "memory");published=1;
    }else if(ci_due(&cinema)){batch.kind=2;batch.words=0;batch.slot=cinema.slot^1u;__asm__ volatile("" ::: "memory");published=1;}
    if(!link && !prefetched && cinema.frame+1u<CI_FRAMES)cinema_request(cinema.frame+1u);
   }
  }
}
void main(void){
 disable_interrupts();stream_install();
#if VIDEO == PAL
 hz=50;
#else
 hz=60;
#endif
 cinema_initialize();enable_interrupts();V16 previous=main_ticks;
 for(;;){
  if(main_ticks==previous)lab_wait();
  V16 delta=(V16)(main_ticks-previous);previous=main_ticks;
  V16 elapsed=delta>50?300:(V16)(delta*(hz==50?6u:5u));
  disable_interrupts();V16 hit=hits;hits=0;V16 done=ack;ack=0;enable_interrupts();
  cinema_acknowledge(done);
  rate_clock=(V16)(rate_clock+elapsed);if(rate_clock>=300){rate_clock-=300;if(playing)images_per_second=rate_count;rate_count=0;}
  ci_tick(&cinema,elapsed);cinema_receive(elapsed);
#ifdef CINEMA_AUTOTEST
  if(!playing && !cinema.done && !stopping)hit|=PAD_A;
#endif
  if(hit&PAD_C)stop_at(0,0);
  if((hit&PAD_START) && playing){cinema.paused^=1u;*ga_reg_comcmd4=cinema.paused?2:1;}
  if(hit&PAD_B)slow^=1u;
  if((hit&PAD_A) && !playing && !link && !published && !ack && !*ga_reg_comstat7){ci_init(&cinema);loaded=0;prefetched=0;stopping=0;failed=0;failure_site=0;playing=1;*ga_reg_comcmd5=0;*ga_reg_comcmd4=1;cinema_request(0);}
  if(stopping && !link && !published && !ack && !*ga_reg_comstat7){loaded=0;prefetched=0;cinema.uploaded=0;}
  cinema_publish();
  if(video_fault)stop_at(CI_FORMAT,10);
  disable_interrupts();if(!published)hud();enable_interrupts();
 }
}

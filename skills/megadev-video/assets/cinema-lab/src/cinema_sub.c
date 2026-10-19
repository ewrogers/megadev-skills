#include "cinema.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/memmap.h>
#include <sub/pcm.h>
#include <system.h>
extern unsigned char const pcm_score[],pcm_chime[];
static V16 copy_tick=0xffffu;
static V16 phase,last_tick,token,frame,result,first,copy_at,sum,expected;
static V16 cache_first[2]={0xffffu,0xffffu},io,io_bank,io_first,io_error,bank;
static char file[]="C00.CIN;1";
static V16 audio_last=0xffffu,audio_state=0xffffu;
static void settle(void){__asm__ volatile("nop\nnop\nnop\nnop" ::: "memory");}
static void sound(V16 channel,V16 start,V16 loop,V16 step,V16 volume){
 *PCM_CTRL=(V8)CHANNEL(channel);settle();*PCM_ENV=(V8)volume;settle();*PCM_PAN=0x88;settle();
 *PCM_FDL=(V8)step;settle();*PCM_FDH=(V8)(step>>8);settle();
 *PCM_LSL=(V8)loop;settle();*PCM_LSH=(V8)(loop>>8);settle();*PCM_ST=(V8)start;settle();
}
void cinema_audio_init(void){
 *PCM_CDISABLE=0xff;
 for(V16 bank=0;bank<9;bank++){
  *PCM_CTRL=(V8)WAVEBANK(bank);settle();
  for(V16 i=0;i<4096;i++)((volatile V8 *)_PCM_RAM)[i*2u+1u]=bank<8?pcm_score[bank*4096u+i]:pcm_chime[i];
 }
 /* 32768-byte loop at 8192 Hz; PCM clock approximation is documented. */
 sound(1,0,0,515,0x70);sound(2,0x80,0x8ffe,515,0xb0);
}
static void audio_tick(void){
 V16 state=*ga_reg_comcmd4,event=*ga_reg_comcmd5;
 if(state!=audio_state){audio_state=state;*PCM_CDISABLE=state==1?0xfe:0xff;settle();}
 if(event!=audio_last){audio_last=event;if(state==1 && event && !(event&15u)){
  *PCM_CDISABLE=0xfe;settle();sound(2,0x80,0x8ffe,515,0xb0);*PCM_CDISABLE=0xfc;settle();
 }}
}
static V16 cache(V16 group){for(V16 i=0;i<2;i++)if(cache_first[i]==group)return i;return 2;}
static void begin_read(V16 group,V16 target){
 V16 n=group>>2;file[1]=(char)('0'+n/10u);file[2]=(char)('0'+n%10u);
 V32 size=stream_file_size(file);io_error=0;
 if(!size){io_error=CI_MISSING;return;}
 if(size!=CI_FILE_BYTES){io_error=CI_FORMAT;return;}
 io_bank=target;io_first=group;cache_first[target]=0xffffu;
 disable_interrupts();filename=file;filebuff=(u8 *)(CI_STAGE+target*CI_FILE_BYTES);filesize=0;access_op_result=0;
 __asm__ volatile("" ::: "memory");access_op=CDROM_LOAD_CDC;enable_interrupts();io=1;
}
static void read_tick(void){
 if(io && access_op==CDROM_IDLE){
  __asm__ volatile("" ::: "memory");io=0;
  if(access_op_result!=CDROM_RESULT_OK)io_error=CI_IO;
  else if(!ci_header((volatile V8 *)(CI_STAGE+io_bank*CI_FILE_BYTES),filesize,io_first))io_error=CI_FORMAT;
  else cache_first[io_bank]=io_first;
 }
 *ga_reg_comstat7=io; /* Cancellation waits for both the mailbox and CDC staging. */
}
void cinema_sub_tick(V16 now){
 if(now!=last_tick){last_tick=now;audio_tick();}
 read_tick();
 if(!phase){
  token=*ga_reg_comcmd0;if(!token || (*ga_reg_memmode&7u)!=2u)return;
  frame=*ga_reg_comcmd1;if(token!=*ga_reg_comcmd0)return;result=CI_OK;copy_at=0;sum=0;
  if(frame>=CI_FRAMES){result=CI_FORMAT;phase=3;}
  else{first=(V16)(frame&~3u);phase=1;}
 }
 if(phase==1){
  bank=cache(first);
  if(bank<2)phase=2;
  else if(!io){
   if(io_error){result=io_error;io_error=0;phase=3;}
   else begin_read(first,0);
  }
 }
 if(phase==2){
  if(copy_tick==now)return;
  copy_tick=now;
  if((*ga_reg_memmode&7u)!=2u)return;
  V32 base=CI_STAGE+bank*CI_FILE_BYTES;
  volatile V8 const *src=(volatile V8 *)(base+2048u+(frame&3u)*CI_BYTES);
  volatile V8 *dst=(volatile V8 *)WORD_RAM_2M;
  V16 count=4096;if(count>CI_BYTES-copy_at)count=CI_BYTES-copy_at;
  for(V16 i=0;i<count;i++){V8 value=src[copy_at+i];dst[copy_at+i]=value;sum=(V16)(sum+value);}
  copy_at+=count;
  if(copy_at<CI_BYTES)return;
  expected=ci_word((volatile V8 *)(base+32u+(frame&3u)*2u));
  if(sum!=expected){result=CI_FORMAT;cache_first[bank]=0xffffu;}
  /* Two fixed PRG buffers: read ahead while this group still supplies frames.
     Never overwrite the group currently being copied or start reads after skip. */
  V16 next=first+CI_GROUP;
  if(result==CI_OK && (frame&3u)>=1 && next<CI_FRAMES && cache(next)==2 && !io && *ga_reg_comcmd4==1)begin_read(next,bank^1u);
  phase=3;
 }
 if(phase==3){*(volatile V8 *)(GA_REG_MEMMODE+1)|=1u;phase=4;}
 if(phase==4){
  if((*ga_reg_memmode&7u)!=1u)return;
  *ga_reg_comstat1=result;*ga_reg_comstat2=frame;*ga_reg_comstat5=sum;*ga_reg_comstat6=token;
  __asm__ volatile("" ::: "memory");*ga_reg_comstat0=token;phase=5;
 }
 if(phase==5 && !*ga_reg_comcmd0){__asm__ volatile("" ::: "memory");*ga_reg_comstat0=0;phase=0;}
}

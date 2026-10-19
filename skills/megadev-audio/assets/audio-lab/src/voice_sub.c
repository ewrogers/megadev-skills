#include "voice.h"
#include "stream.h"
#include "../build/voice_assets.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/pcm.h>
#include <system.h>
VoiceStream voice_stream;
/* One staging buffer, regardless of clip length. CDC writes complete sectors. */
static unsigned char staging[VS_HALF];
static char chunk_name[]="V000.PCM;1";
static A16 source,slow,io,upload,half,last_tick,age,locked,failed;
static A16 const blocks[]={VOICE_V_BLOCKS,VOICE_L_BLOCKS};
static A16 const * const sums[]={voice_v_sums,voice_l_sums};
static A16 checksum;
static void settle(void){__asm__ volatile("nop\nnop\nnop\nnop" ::: "memory");}
static void select_voice(void){*PCM_CTRL=(unsigned char)CHANNEL(4);settle();}
static void set_loop(A16 loop) {
    select_voice();*PCM_LSL=(unsigned char)loop;settle();*PCM_LSH=(unsigned char)(loop>>8);settle();
}
static void play(A16 h) {
    audio_stream_enable(0);select_voice();
    *PCM_ENV=0x90;settle();*PCM_PAN=0x88;settle();
    *PCM_FDL=(unsigned char)VOICE_STEP;settle();*PCM_FDH=(unsigned char)(VOICE_STEP>>8);settle();
    set_loop(voice_loop(&voice_stream));
    *PCM_ST=(unsigned char)(h?0x80:0x40);settle();audio_stream_enable(1);
}
static A16 cursor(void) {
    /* The sample pointer crosses byte boundaries independently of the Sub CPU. */
    volatile unsigned char const *p=(volatile unsigned char const *)_PCM_PLAY_CH4_L;
    for(A16 retry=0;retry<3;retry++) {
        A16 hi=p[2],lo=p[0];
        if(hi==p[2])return (A16)((hi<<8)|lo);
    }
    return 0xffffu;
}
int voice_busy(void){return locked || io || voice_stream.phase==VS_BUFFER || voice_stream.phase==VS_PLAY || voice_stream.phase==VS_DRAIN;}
A16 voice_status(void){return (A16)((voice_stream.phase<<4)|(voice_stream.underruns?0x80u:0));}
void voice_request(A16 sample,A16 test_slow) {
    if(sample>1 || voice_busy() || audio_state.data_lock || *ga_reg_comcmd0) {voice_stream.rejected++;return;}
    if(!voice_begin(&voice_stream,blocks[sample]))return;
    source=sample;slow=test_slow;upload=0;failed=0;age=0;
}
void voice_stop(void) {
    if(!voice_busy())return;
    audio_stream_enable(0);voice_stream.phase=VS_DRAIN;
}
static void fault(void){failed=1;voice_stop();}
void voice_sub_tick(A16 now,A16 hz) {
    if(now==last_tick)return;
    A16 delta=(A16)(now-last_tick);last_tick=now;
    if(!voice_busy())return;
    /* Arbitrary worker stalls are not safe streaming: stop before any refill. */
    if(delta>hz && voice_stream.phase==VS_PLAY)fault();
    if(voice_stream.phase==VS_DRAIN) {
        if(io && access_op!=CDROM_IDLE)return; /* Cancel does not cancel CDC. */
        io=0;upload=0;
        if(locked){audio_drive_release();locked=0;}
        voice_stream.phase=failed?VS_FAULT:VS_IDLE;return;
    }
    if(voice_stream.phase==VS_PLAY) {
        A16 pos=cursor();
        A16 h=pos>=0x4000u && pos<0x8000u?0:pos>=0x8000u && pos<0xc000u?1:pos>=VS_SILENCE && pos<=VS_SILENCE+1u?2:3;
        voice_observe(&voice_stream,h);
        if(voice_stream.phase==VS_FAULT){fault();return;}
        /* Retire the consumed half and disarm stale loops before loading it. */
        set_loop(voice_loop(&voice_stream));
        if(voice_stream.phase==VS_DONE) {
            audio_stream_enable(0);if(locked){audio_drive_release();locked=0;}return;
        }
    }
    /* Reserve before waiting for CD-DA to stop; cancellation must release even
       a reservation whose stop command has not completed yet. */
    locked=1;
    if(audio_state.fault){fault();return;}
    if(!audio_drive_acquire())return;
    if(io) {
        if(access_op!=CDROM_IDLE) {
            if(delta>=hz*30u-age){age=hz*30u;fault();}else age+=delta;
            return;
        }
        __asm__ volatile("" ::: "memory");io=0;
        if(access_op_result!=CDROM_RESULT_OK || filesize!=VS_HALF){fault();return;}
        upload=1;checksum=0;
    }
    if(upload) {
        A16 offset=(A16)(upload-1u),count=slow?64u:512u;
        if(count>VS_HALF-offset)count=(A16)(VS_HALF-offset);
        /* Never modify the currently playing half, even after a delayed read. */
        if(half==voice_stream.current){fault();return;}
        A16 bank=(A16)(4u+half*4u+(offset>>12));
        *PCM_CTRL=(unsigned char)WAVEBANK(bank);settle();
        for(A16 i=0;i<count;i++) {
            A16 at=(A16)(offset+i);unsigned char value=staging[at];
            if((value==0xff)!=(at==VS_HALF-1u)){fault();return;}
            checksum=(A16)(checksum+value);
            ((volatile unsigned char *)_PCM_RAM)[((at&4095u)*2u)+1u]=value;
        }
        offset+=count;
        if(offset==VS_HALF) {
            upload=0;
            if(checksum!=sums[source][voice_stream.next] || !voice_commit(&voice_stream,half)){fault();return;}
        } else upload=(A16)(offset+1u);
    }
    int resume=voice_resume(&voice_stream);
    if(resume>=0)play((A16)resume);
    else if(voice_stream.phase==VS_PLAY)set_loop(voice_loop(&voice_stream));
    if(!io && !upload) {
        int target=voice_target(&voice_stream);
        if(target<0)return;
        half=(A16)target;
        A16 n=voice_stream.next;
        chunk_name[0]=source?'L':'V';chunk_name[1]=(char)('0'+n/100u);
        chunk_name[2]=(char)('0'+(n/10u)%10u);chunk_name[3]=(char)('0'+n%10u);
        if(stream_file_size(chunk_name)!=VS_HALF || access_op!=CDROM_IDLE){fault();return;}
        disable_interrupts();filename=chunk_name;filebuff=staging;filesize=0;access_op_result=0;
        __asm__ volatile("" ::: "memory");access_op=CDROM_LOAD_CDC;enable_interrupts();io=1;age=0;
    }
}

#include "audio.h"
#include "../build/audio_assets.h"
#include <sub/bios.def.h>
#include <sub/gate_arr.h>
#include <sub/pcm.h>
#include <system.h>
AudioState audio_state;
extern unsigned char const pcm_bed[],pcm_bell[],pcm_pluck[],pcm_drum[];
static A16 last_tick,mail_stage,drive_elapsed,bed_applied;
static unsigned char disabled=0xff;
static void settle(void){__asm__ volatile("nop\nnop\nnop\nnop" ::: "memory");}
static unsigned char pan_byte(A16 pan){return pan==0?0x0c:pan==2?0xc0:0x88;}
static void channel(A16 index,A16 sample,A16 pan) {
    disabled|=(unsigned char)(1u<<index);*PCM_CDISABLE=disabled;settle();
    *PCM_CTRL=(unsigned char)CHANNEL(index+1);settle();
    *PCM_ENV=index?0x90:0x48;settle();*PCM_PAN=pan_byte(pan);settle();
    A16 step=index?AUDIO_BELL_STEP:AUDIO_BED_STEP;
    *PCM_FDL=(unsigned char)step;settle();*PCM_FDH=(unsigned char)(step>>8);settle();
    A16 loop=index?0x3f00:0;
    *PCM_LSL=(unsigned char)loop;settle();*PCM_LSH=(unsigned char)(loop>>8);settle();
    *PCM_ST=(unsigned char)(sample<<4);settle();
    disabled&=(unsigned char)~(1u<<index);*PCM_CDISABLE=disabled;settle();
}
void audio_init(void) {
    *PCM_CDISABLE=0xff;audio_state.pan=1;
    unsigned char const *data[]={pcm_bed,pcm_bell,pcm_pluck,pcm_drum};
    A16 const sizes[]={AUDIO_BED_BYTES,AUDIO_BELL_BYTES,AUDIO_PLUCK_BYTES,AUDIO_DRUM_BYTES};
    /* Own these four banks; no PCM RAM upload is performed while voices use them. */
    for(A16 bank=0;bank<4;bank++) {
        *PCM_CTRL=(unsigned char)WAVEBANK(bank);settle();
        for(A16 i=0;i<4096;i++)((volatile unsigned char *)_PCM_RAM)[i*2u+1u]=i<sizes[bank]?data[bank][i]:0x80;
        ((volatile unsigned char *)_PCM_RAM)[sizes[bank]*2u+1u]=0xff;
    }
    *PCM_CTRL=(unsigned char)WAVEBANK(3);settle();
    ((volatile unsigned char *)_PCM_RAM)[0xf00u*2u+1u]=0x80;
    ((volatile unsigned char *)_PCM_RAM)[0xf01u*2u+1u]=0xff;
}
int audio_drive_acquire(void) {
    audio_state.data_lock=1;
    return !audio_state.fault && !audio_state.inflight && !audio_state.applied_track;
}
void audio_drive_release(void){audio_state.data_lock=0;}
void audio_sub_tick(A16 now,A16 hz) {
    if(now==last_tick)return;
    A16 delta=(A16)(now-last_tick);last_tick=now;
    A16 elapsed=delta>=(A16)(hz*30u)?9000:(hz==50?(A16)(delta*6u):(A16)(delta*5u));
    audio_tick(&audio_state,elapsed);
    for(A16 i=0;i<2;i++)if(audio_state.ended_mask&(1u<<i))disabled|=(unsigned char)(1u<<(i+1));
    *PCM_CDISABLE=disabled;
    A16 seq=*ga_reg_comcmd4;
    if(!mail_stage && seq) {
        A16 op=*ga_reg_comcmd5,arg=*ga_reg_comcmd6;
        if(seq!=*ga_reg_comcmd4)return;
        if(op==AU_EFFECT && arg<3) {
            A16 const ticks[]={AUDIO_BELL_TICKS,AUDIO_PLUCK_TICKS,AUDIO_DRUM_TICKS};
            int voice=audio_voice(&audio_state,arg,(A16)(arg+1),ticks[arg],audio_state.pan);
            if(voice>=0)channel((A16)(voice+1),(A16)(arg+1),audio_state.pan);
        } else {
            audio_command(&audio_state,op,arg);
            if(op==AU_PAN) {
                for(A16 i=0;i<3;i++){*PCM_CTRL=(unsigned char)CHANNEL(i+1);settle();*PCM_PAN=pan_byte(audio_state.pan);settle();}
            }
            if(op==AU_PANIC){disabled=0xff;bed_applied=0;*PCM_CDISABLE=disabled;}
        }
        __asm__ volatile("" ::: "memory");*ga_reg_comstat7=seq;mail_stage=1;
    }
    if(mail_stage && !*ga_reg_comcmd4){*ga_reg_comstat7=0;mail_stage=0;}
    if(audio_state.bed!=bed_applied) {
        bed_applied=audio_state.bed;
        if(bed_applied)channel(0,0,audio_state.pan);
        else {disabled|=1;*PCM_CDISABLE=disabled;}
    }
    if(audio_state.inflight) {
        if(audio_cd_ready()){audio_drive_complete(&audio_state);drive_elapsed=0;}
        else if(elapsed>=9000u-drive_elapsed){audio_state.fault=1;drive_elapsed=9000;}
        else drive_elapsed+=elapsed;
    }
    A16 next=audio_drive_next(&audio_state);
    if(next) {
        A16 command=next==CD_STOP?BIOS_MSC_STOP:next==CD_PLAY?BIOS_MSC_PLAYR:next==CD_PAUSE?BIOS_MSC_PAUSEON:BIOS_MSC_PAUSEOFF;
        audio_cd_command(command,audio_state.pending_track);drive_elapsed=0;
    }
    A16 state=audio_state.fault?5:audio_state.data_lock?3:audio_state.inflight?4:!audio_state.applied_track?0:audio_state.applied_pause?2:1;
    *ga_reg_comstat4=(A16)((state<<8)|((unsigned char)~disabled&7u));
}

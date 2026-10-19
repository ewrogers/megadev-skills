#include "stream.h"
#include "audio.h"
#include "voice.h"
#include "../build/stream_assets.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/memmap.h>
#include <system.h>
static S16 stage,last_tick,token,asset,result;
static S32 length;
static char const * const files[]={"GARDEN.MDA;1","CAVERN.MDA;1","MISSING.MDA;1"};
static S16 const sizes[]={STREAM_GARDEN_BYTES,STREAM_CAVERN_BYTES,0};
void stream_sub_tick(S16 now,S16 hz) {
    (void)hz;
    if(now==last_tick)return;
    last_tick=now;
    if(!stage) {
        if(voice_busy())return; /* One drive owner; voice keeps its reservation. */
        token=*ga_reg_comcmd0;if(!token)return;
        asset=*ga_reg_comcmd1;if(token!=*ga_reg_comcmd0)return;
        result=SR_OK;length=0;
        if((*ga_reg_memmode&7u)!=2u)return; /* No speculative writes before grant. */
        if(!audio_drive_acquire())return;
        if(asset>2){result=SR_SIZE;stage=2;}
        else {
            length=stream_file_size(files[asset]);
            if(!length){result=SR_MISSING;stage=2;}
            else if(!stream_rounded(length,STREAM_CAP) || length!=sizes[asset]){result=SR_SIZE;stage=2;}
            else {
                /* Parameters first, operation last; INT2 must never see a half request. */
                disable_interrupts();filename=files[asset];filebuff=(u8 *)WORD_RAM_2M;
                filesize=0;access_op_result=0;
                __asm__ volatile("" ::: "memory");access_op=CDROM_LOAD_CDC;enable_interrupts();stage=1;
            }
        }
    }
    if(stage==1) {
        if(access_op!=CDROM_IDLE)return;
        __asm__ volatile("" ::: "memory");
        if(access_op_result!=CDROM_RESULT_OK)result=SR_IO;
        else if(filesize!=length)result=SR_SIZE;
        stage=2;
    }
    if(stage==2) {
        audio_drive_release();
        /* Even cancelled reads must finish before returning ownership. */
        *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=1u;stage=3;
    }
    if(stage==3) {
        if((*ga_reg_memmode&7u)!=1u)return;
        *ga_reg_comstat1=result;*ga_reg_comstat2=(S16)length;*ga_reg_comstat5=asset;*ga_reg_comstat6=token;
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=token;stage=4;
    }
    if(stage==4 && !*ga_reg_comcmd0) {
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=0;stage=0;
    }
}

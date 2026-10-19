#include "stream.h"
#include "compress.h"
#include "../build/compression_assets.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/memmap.h>
#include <system.h>
static S16 stage,last_tick,token,asset,result,read_start,read_frames,decode_ticks;
static S32 length,output;
void stream_sub_tick(S16 now,S16 hz) {
    if(now==last_tick)return;
    last_tick=now;
    if(!stage) {
        token=*ga_reg_comcmd0;if(!token)return;
        asset=*ga_reg_comcmd1;if(token!=*ga_reg_comcmd0)return;
        result=SR_OK;length=0;output=0;read_frames=0;decode_ticks=0;
        if((*ga_reg_memmode&7u)!=2u)return;
        if(asset>=6){result=SR_MISSING;stage=2;}
        else {
            length=stream_file_size(cp_files[asset]);output=cp_outputs[asset];
            if(!length){result=SR_MISSING;stage=2;}
            else if(!stream_rounded(length,STREAM_CAP) || length!=cp_sizes[asset]){result=SR_SIZE;stage=2;}
            else {
                disable_interrupts();filename=cp_files[asset];filebuff=(u8 *)WORD_RAM_2M;
                filesize=0;access_op_result=0;read_start=now;
                __asm__ volatile("" ::: "memory");access_op=CDROM_LOAD_CDC;enable_interrupts();stage=1;
            }
        }
    }
    if(stage==1) {
        if(access_op!=CDROM_IDLE)return;
        __asm__ volatile("" ::: "memory");read_frames=(S16)(now-read_start);
        if(access_op_result!=CDROM_RESULT_OK)result=SR_IO;
        else if(filesize!=length)result=SR_SIZE;
        else {
            volatile u8 const *src=(volatile u8 *)WORD_RAM_2M;
            volatile u8 *dst=(volatile u8 *)(WORD_RAM_2M+CP_OUTPUT);
            /* Input stays immutable and exclusively Sub-owned after the reader
               retires. The stock decoder itself has no bounds parameters. */
            if((asset&1u) && !cp_kos_valid(src,length,output))result=SR_FORMAT;
            else {
                S16 before_frame=*ga_reg_comcmd3;
                S16 before=*(volatile S16 *)GA_REG_STOPWATCH;
                if(asset&1u)cp_kos_decode(src,dst);
                else cp_raw_copy(src,dst,output);
                S16 after=*(volatile S16 *)GA_REG_STOPWATCH;
                decode_ticks=cp_elapsed(before,after,(S16)(*ga_reg_comcmd3-before_frame),hz);
            }
        }
        stage=2;
    }
    if(stage==2) {
        /* Cancellation never steals a buffer from the live CDC operation. */
        *(volatile u8 *)(GA_REG_MEMMODE+1)|=1u;stage=3;
    }
    if(stage==3) {
        if((*ga_reg_memmode&7u)!=1u)return;
        *ga_reg_comstat1=result;*ga_reg_comstat2=(S16)output;*ga_reg_comstat4=read_frames;
        *ga_reg_comstat5=asset;*ga_reg_comstat6=token;*ga_reg_comstat7=decode_ticks;
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=token;stage=4;
    }
    if(stage==4 && !*ga_reg_comcmd0){__asm__ volatile("" ::: "memory");*ga_reg_comstat0=0;stage=0;}
}

#include "stream.h"
#include "compress.h"
#include <main/gate_arr.h>
#include <main/memmap.h>
S16 stream_phase,stream_ready,stream_asset,stream_result,stream_pass,stream_dropped,stream_errors,stream_timeouts;
S16 stream_uploaded,stream_cache_map[STREAM_MAP],stream_cache_palette[16],stream_cache_tiles;
static S16 serial=1,token,cancelled,elapsed_total,received;
static StreamImage image;
S16 stream_decode_ticks,stream_read_frames;
static void fault(void){stream_errors++;stream_phase=ST_FAULT;stream_ready=0;}
int stream_start(S16 asset) {
    if(stream_phase || *ga_reg_comcmd0 || *ga_reg_comstat0 || asset>6 || (*ga_reg_memmode&7u)!=1u)return 0;
    serial++;if(serial<2)serial=2;token=serial;stream_asset=asset;
    cancelled=0;elapsed_total=0;stream_ready=0;stream_uploaded=0;stream_result=0;
    *ga_reg_comcmd1=asset;__asm__ volatile("" ::: "memory");
    *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=2u;stream_phase=ST_GRANT;return 1;
}
void stream_cancel(void) {
    if(stream_phase==ST_FAULT)return;
    if(stream_phase==ST_GRANT || stream_phase==ST_READ || stream_phase==ST_DRAIN || stream_phase==ST_RETIRE){cancelled=1;return;}
    if(stream_phase || stream_ready){stream_dropped++;stream_phase=ST_IDLE;stream_ready=0;stream_result=SR_CANCEL;}
}
void stream_main_tick(S16 elapsed) {
    if(stream_phase==ST_IDLE || stream_phase==ST_FAULT || stream_phase==ST_UPLOAD)return;
    /* Observe the committed reply before sampling its returned ownership. */
    S16 reply=*ga_reg_comstat0;
    S16 mode=*ga_reg_memmode;
    if(stream_phase==ST_GRANT || stream_phase==ST_READ || stream_phase==ST_RETIRE) {
        if(elapsed>=STREAM_TIMEOUT-elapsed_total)elapsed_total=STREAM_TIMEOUT;
        else elapsed_total=(S16)(elapsed_total+elapsed);
        if(elapsed_total==STREAM_TIMEOUT && stream_result!=SR_TIMEOUT) {
            stream_timeouts++;cancelled=1;stream_result=SR_TIMEOUT;
            if(stream_phase!=ST_RETIRE)stream_phase=ST_DRAIN;
        }
    }
    if(stream_phase==ST_GRANT || (stream_phase==ST_DRAIN && !*ga_reg_comcmd0)) {
        if((mode&7u)==2u){__asm__ volatile("" ::: "memory");*ga_reg_comcmd0=token;if(stream_phase!=ST_DRAIN)stream_phase=ST_READ;}
        return;
    }
    if(stream_phase==ST_READ || stream_phase==ST_DRAIN) {
        if(reply!=token)return;
        if((mode&7u)!=1u || *ga_reg_comstat6!=token || *ga_reg_comstat5!=stream_asset){fault();return;}
        received=*ga_reg_comstat2;stream_decode_ticks=*ga_reg_comstat7;stream_read_frames=*ga_reg_comstat4;
        if(stream_result!=SR_TIMEOUT)stream_result=cancelled?SR_CANCEL:*ga_reg_comstat1;
        __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=0;stream_phase=ST_RETIRE;return;
    }
    if(stream_phase==ST_RETIRE) {
        if(reply)return;
        if((mode&7u)!=1u){fault();return;}
        if(cancelled || stream_result!=SR_OK) {
            stream_dropped++;stream_phase=ST_IDLE;return;
        }
        if(!stream_image_begin(&image,(volatile unsigned char *)(WORD_RAM+CP_OUTPUT),received)) {
            stream_result=SR_FORMAT;stream_errors++;stream_phase=ST_IDLE;return;
        }
        stream_phase=ST_VALIDATE;
    }
    if(stream_phase==ST_VALIDATE) {
        if((mode&7u)!=1u){fault();return;}
        int result=stream_image_step(&image,(volatile unsigned char *)(WORD_RAM+CP_OUTPUT),128);
        if(result<0){stream_result=SR_FORMAT;stream_errors++;stream_phase=ST_IDLE;return;}
        if(result) {
            volatile unsigned char const *p=(volatile unsigned char *)(WORD_RAM+CP_OUTPUT);
            for(S16 i=0;i<16;i++)stream_cache_palette[i]=stream_word(p+32+i*2u);
            for(S16 i=0;i<STREAM_MAP;i++)stream_cache_map[i]=stream_word(p+image.map_offset+i*2u);
            stream_cache_tiles=image.tiles;stream_phase=ST_UPLOAD;
        }
    }
}
/* Call only after the publisher consumed the copied payload; no pointer is queued. */
void stream_upload_done(S16 bytes) {
    if(stream_phase!=ST_UPLOAD || !bytes || (bytes&1u) || bytes>STREAM_CHUNK ||
       stream_uploaded+bytes>(S32)stream_cache_tiles*32u){fault();return;}
    stream_uploaded=(S16)(stream_uploaded+bytes);
    if(stream_uploaded==(S32)stream_cache_tiles*32u){stream_phase=ST_IDLE;stream_ready=1;stream_pass++;}
}

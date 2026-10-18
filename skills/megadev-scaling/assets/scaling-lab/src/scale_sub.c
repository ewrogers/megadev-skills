#include "scale.h"
#include <sub/gate_arr.h>
#include <sub/memmap.h>
extern unsigned char const scale_art[];
static ScU16 stage,token,result;
static ScLine trace[SC_HEIGHT];
/* No ASIC interrupt handler: completion is polled in Sub foreground. */
void scale_sub_tick(void) {
    if(!stage) {
        token=*ga_reg_comcmd0;if(token<2)return;
        ScRequest request={*ga_reg_comcmd1,*ga_reg_comcmd2,*ga_reg_comcmd3,*ga_reg_comcmd4};
        if(token!=*ga_reg_comcmd0)return;
        /* Ownership must be sampled AFTER the committed request token. The
           Main grant/token may arrive between two Sub polling instructions. */
        ScU16 mode=*ga_reg_memmode;
        result=1;
        if((mode&7u)!=2u || (ga_reg_stampsize&SC_BUSY) || !scale_trace(trace,SC_HEIGHT,&request)) {result=2;stage=4;}
        else {
            /* Rebuild source only on the first acquired frame. Word RAM held
               the resident bootstrap before this runtime protocol began. */
            static ScU16 initialized;
            volatile ScU16 *ram=(volatile ScU16 *)WORD_RAM_2M;
            if(!initialized) {
                ScU16 const *src=(ScU16 const *)scale_art;
                for(ScU16 i=0;i<16448;i++)ram[i]=src[i];
                for(ScU16 i=0;i<256;i++)ram[SC_MAP/2+i]=(ScU16)(i+1u);
                initialized=1;
            }
            volatile ScLine *dest=(volatile ScLine *)(WORD_RAM_2M+SC_TRACE);
            for(ScU16 i=0;i<SC_HEIGHT;i++)dest[i]=trace[i];
            /* Overwrite priority, 2M mode, 16px stamps / 256px map, repeat.
               These raw offsets and units are hardware, not C helper APIs. */
            *(volatile unsigned char *)(GA_REG_MEMMODE+1)=(unsigned char)(mode&~0x18u);
            /* This owner polls completion: disable only the ASIC INT1.
               Preserve the BIOS/CD INT2/4/5/6 and any timer mask bits. */
            *(volatile unsigned char *)(GA_REG_INTMASK+1)&=(unsigned char)~(1u<<BIT_GA_REG_INT1);
            ga_reg_stampsize=1;
            ga_reg_stampmapbase=SC_MAP/4;
            ga_reg_imgbufvsize=SC_HEIGHT/8-1;
            ga_reg_imgbufstart=SC_IMAGE/4;
            ga_reg_imgbufoffset=0;
            ga_reg_imgbufhdotsize=SC_WIDTH;
            ga_reg_imgbufvdotsize=SC_HEIGHT;
            __asm__ volatile("" ::: "memory");
            ga_reg_tracevectbase=SC_TRACE/4; /* Writing starts the ASIC. */
            stage=1;return;
        }
    }
    if(stage==1) {
        if((*ga_reg_memmode&7u)!=2u){result=3;stage=4;return;}
        if(ga_reg_stampsize&SC_BUSY)return;
        __asm__ volatile("" ::: "memory");
        *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=1u;stage=2;
    }
    if(stage==2){if((*ga_reg_memmode&7u)!=1u)return;stage=4;}
    if(stage==4) {
        *ga_reg_comstat1=result;
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=token;stage=5;
    }
    if(stage==5 && !*ga_reg_comcmd0){*ga_reg_comstat0=0;stage=0;}
}

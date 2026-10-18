#include "scale.h"
#include <main/gate_arr.h>
#include <main/memmap.h>
ScU16 scale_phase,scale_error,scale_completed,scale_generation;
volatile ScU16 scale_upload,scale_last_mode;
ScU16 scale_pixels[SC_WORDS];
static ScU16 serial=1,age,issued_mode;
extern void scaling_copy_image(void const *source,void *destination);
int scale_start(ScRequest const *r) {
    if(scale_phase || *ga_reg_comcmd0 || *ga_reg_comstat0 || (*ga_reg_memmode&7u)!=1u || !scale_valid(r))return 0;
    serial++;if(serial<2)serial=2;age=0;issued_mode=r->mode;
    *ga_reg_comcmd1=r->mode;*ga_reg_comcmd2=r->angle;*ga_reg_comcmd3=r->zoom;*ga_reg_comcmd4=r->motion;
    __asm__ volatile("" ::: "memory");
    *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=2u;scale_phase=1;scale_main_tick(0);return 1;
}
static void fault(void){scale_error++;scale_phase=4;}
void scale_main_tick(ScU16 elapsed) {
    if(!scale_phase || scale_phase==4)return;
    if(elapsed>=SC_TIMEOUT-age){fault();return;}age+=elapsed;
    ScU16 mode=*ga_reg_memmode;
    if(scale_phase==1) {
        if((mode&7u)==2u){__asm__ volatile("" ::: "memory");*ga_reg_comcmd0=serial;scale_phase=2;}return;
    }
    if(scale_phase==2) {
        if(*ga_reg_comstat0!=serial)return;
        if(*ga_reg_comstat1!=1 || (*ga_reg_memmode&7u)!=1u){fault();return;}
        /* Copy completed pixels to a resident Main snapshot. VBlank never
           retains a Word RAM pointer or races the next ASIC operation. */
        volatile ScU16 const *src=(volatile ScU16 const *)(WORD_RAM+SC_IMAGE);
        /* The next ASIC job can finish while the prior snapshot is being
           uploaded, but cannot replace that snapshot until its final DMA. */
        if(scale_upload)return;
        scaling_copy_image((void const *)src,scale_pixels);
        __asm__ volatile("" ::: "memory");scale_generation++;scale_last_mode=issued_mode;scale_upload=1;
        *ga_reg_comcmd0=0;scale_phase=3;
    }
    if(scale_phase==3 && !*ga_reg_comstat0){scale_phase=0;scale_completed++;}
}

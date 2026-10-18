#include "link.h"
#include <main/gate_arr.h>
#include <main/memmap.h>
SyClient client;
SyU16 job_seed,passed,rejected,drained,verify_failed;
static SyU16 sequence=1;
int sy_main_start(SyU16 profile,SyU16 slow) {
    SyU16 mode=*ga_reg_memmode;
    SyU16 next=(SyU16)(sequence+1u);if(next<2)next=2;
    if(!sy_begin(&client,next,300,!*ga_reg_comcmd0 && !*ga_reg_comstat0,(mode&7u)==1u))return 0;
    sequence=next;job_seed=(SyU16)(next^0x39C6u);
    sy_prepare((volatile SyU16 *)WORD_RAM,next,job_seed);
    *ga_reg_comcmd1=profile==1?0x77:SY_BUILD;
    *ga_reg_comcmd2=job_seed;*ga_reg_comcmd3=profile==2?257:SY_WORDS;
    *ga_reg_comcmd4=0;*ga_reg_comcmd5=slow?1:0;
    __asm__ volatile("" ::: "memory");
    /* One request, then frame-by-frame observation; no unbounded GRANT_2M loop. */
    *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=2;
    return 1;
}
void sy_main_cancel(void){sy_cancel(&client);}
void sy_main_tick(SyU16 elapsed) {
    SyU16 token=*ga_reg_comstat0;
    SyU16 code=*ga_reg_comstat1;
    SyU16 mode=*ga_reg_memmode;
    int event=sy_poll(&client,token,code,mode,elapsed);
    if(event==1) {
        __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=client.token;
    }
    if(client.phase==SY_DRAIN)*ga_reg_comcmd4=client.token;
    if(event==2) {
        /* Ownership and commit token were checked before touching returned RAM. */
        if(client.discard)drained++;
        else if(code!=SY_OK)rejected++;
        else if(*ga_reg_comstat6!=client.token || !sy_verify((volatile SyU16 *)WORD_RAM,client.token,job_seed,*ga_reg_comstat5))verify_failed++;
        else passed++;
        *ga_reg_comcmd4=0;
        __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=0;
    }
}

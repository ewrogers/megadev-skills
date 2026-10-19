#include "link.h"
#include <sub/gate_arr.h>
#include <sub/memmap.h>
static SyWorker worker;
static SyU16 state,last_tick,delay;
void sy_sub_tick(SyU16 now,SyU16 hz) {
    /* Called by foreground, not the BIOS INT2 access-loop callback. */
    if(now==last_tick)return;
    last_tick=now;
    if(!state) {
        SyU16 token=*ga_reg_comcmd0;if(!token)return;
        SyU16 opcode=*ga_reg_comcmd1,seed=*ga_reg_comcmd2,count=*ga_reg_comcmd3,slow=*ga_reg_comcmd5;
        if(token!=*ga_reg_comcmd0)return;
        sy_worker_begin(&worker,token,opcode,seed,count);
        if(slow>1){worker.result=SY_BAD_REQUEST;worker.phase=SY_RETIRE;}
        delay=slow?(SyU16)(hz+hz):0;state=1;
    }
    if(state==1) {
        /* Deliberately delayed job models a service that cannot abort immediately. */
        if(delay){delay--;*ga_reg_comstat7=SY_DRAIN;return;}
        sy_worker_step(&worker,(volatile SyU16 *)WORD_RAM_2M,(*ga_reg_memmode&7u)==2u,*ga_reg_comcmd4==worker.token);
        *ga_reg_comstat2=worker.done;*ga_reg_comstat7=SY_WAIT;
        if(worker.phase!=SY_RETIRE)return;
        __asm__ volatile("" ::: "memory");
        /* A one-shot request replaces the SDK helper's indefinite confirmation loop. */
        if((*ga_reg_memmode&7u)==2u)*(volatile unsigned char *)(GA_REG_MEMMODE+1)|=1;
        state=2;
    }
    if(state==2) {
        if((*ga_reg_memmode&7u)!=1u)return;
        *ga_reg_comstat1=worker.result;*ga_reg_comstat5=worker.sum;
        *ga_reg_comstat6=worker.token;*ga_reg_comstat7=SY_RETIRE;
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=worker.token;state=3;
    }
    if(state==3 && !*ga_reg_comcmd0) {
        *ga_reg_comstat2=0;*ga_reg_comstat7=SY_IDLE;
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=0;state=0;
    }
}

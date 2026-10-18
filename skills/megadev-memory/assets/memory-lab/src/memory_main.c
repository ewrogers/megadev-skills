#include "memory_link.h"
#include <main/gate_arr.h>
#include <main/memmap.h>
MView word_view;
M16 memory_phase,front_valid,back_ready,front_seed,front_token,back_seed,back_token;
M16 memory_pass,memory_errors,memory_blocked,memory_progress;
M16 memory_preview[64];
static M16 serial=1,opcode,token,seed,old_ret,elapsed_total,exit_granted;
/* Phase 1 grant, 2 reply, 3 retirement, 4 quarantined fault. */
int memory_start(M16 command) {
    M16 mode=*ga_reg_memmode;
    if(memory_phase || *ga_reg_comcmd0 || *ga_reg_comstat0 || command<1 || command>5)return 0;
    if(((command==MM_FILL2 || command==MM_ENTER1) && (mode&7u)!=1u) ||
       ((command==MM_FILL1 || command==MM_SWAP1 || command==MM_EXIT2) && !(mode&4u)))return 0;
    if(command==MM_SWAP1 && !back_ready)return 0;
    if(command!=MM_FILL1 && !mv_begin(&word_view)){memory_blocked++;return 0;}
    if(command!=MM_FILL1)front_valid=0;
    serial++;if(serial<2)serial=2;token=serial;opcode=command;seed=(M16)(0x39C6u^token);
    old_ret=mode&1u;elapsed_total=0;exit_granted=0;memory_progress=0;
    *ga_reg_comcmd1=command;*ga_reg_comcmd2=seed;*ga_reg_comcmd3=old_ret;
    __asm__ volatile("" ::: "memory");
    if(command==MM_FILL2 || command==MM_ENTER1) {
        *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=2;memory_phase=1;
    } else {
        if(command==MM_SWAP1) {
            /* In 1M mode Main requests exchange by clearing DMNA. Sub changes RET. */
            *(volatile unsigned char *)(GA_REG_MEMMODE+1)&=(unsigned char)~2u;
        }
        *ga_reg_comcmd0=token;memory_phase=2;
    }
    return 1;
}
static void fault(void){memory_errors++;memory_phase=4;front_valid=0;}
int memory_front_check(void) {
    if(!front_valid || word_view.pending)return 0;
    M16 mode=*ga_reg_memmode;
    if((mode&4u)==0 && (mode&7u)!=1u){fault();return 0;}
    MLease lease={0};if(!mv_pin(&word_view,&lease))return 0;
    /* Same Main linear window; the mode/RET determines its physical backing. */
    int good=memory_verify((volatile M16 *)WORD_RAM,front_seed,front_token);
    if(good)for(M16 i=0;i<64;i++)memory_preview[i]=((volatile M16 *)WORD_RAM)[4+i];
    if((*ga_reg_memmode&7u)!=(mode&7u))good=0;
    mv_release(&word_view,&lease);
    if(!good)fault();
    return good;
}
void memory_main_tick(M16 elapsed) {
    if(!memory_phase || memory_phase==4)return;
    M16 mode=*ga_reg_memmode,reply=*ga_reg_comstat0;
    if(memory_phase==3) {
        if(!reply){memory_phase=0;memory_pass++;}return;
    }
    if(elapsed>=1500u-elapsed_total)elapsed_total=1500;
    else elapsed_total=(M16)(elapsed_total+elapsed);
    if(elapsed_total==1500){fault();return;}
    if(memory_phase==1) {
        if((mode&7u)==2u){__asm__ volatile("" ::: "memory");*ga_reg_comcmd0=token;memory_phase=2;}
        return;
    }
    /* After MODE clears, grant in actual 2M mode if that intermediate owner is Main. */
    if(opcode==MM_EXIT2 && !exit_granted && (mode&7u)==1u && !reply) {
        *(volatile unsigned char *)(GA_REG_MEMMODE+1)|=2;exit_granted=1;
    }
    memory_progress=*ga_reg_comstat2;
    if(reply!=token)return;
    if(*ga_reg_comstat1!=1){fault();return;}
    M16 expected=(opcode==MM_FILL2 || opcode==MM_EXIT2)?1u:
        (opcode==MM_ENTER1?4u:(M16)(4u|(opcode==MM_SWAP1?(old_ret^1u):old_ret)));
    if((mode&7u)!=expected){fault();return;}
    if(opcode==MM_ENTER1) {
        /* Rebuild after layout change. Old 2M contents are not two linear 1M halves. */
        front_seed=0x1111;front_token=token;
        memory_fill((volatile M16 *)WORD_RAM,front_seed,front_token,0,M_WORDS);
        back_seed=seed;back_token=token;back_ready=1;
    } else if(opcode==MM_FILL1){back_seed=seed;back_token=token;back_ready=1;}
    else if(opcode==MM_SWAP1){front_seed=back_seed;front_token=back_token;back_ready=0;}
    else {front_seed=seed;front_token=token;back_ready=0;}
    if(opcode!=MM_FILL1)mv_complete(&word_view);
    front_valid=1;
    if(!memory_front_check())return;
    __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=0;memory_phase=3;
}

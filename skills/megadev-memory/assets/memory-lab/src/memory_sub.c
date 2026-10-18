#include "memory_link.h"
#include <sub/gate_arr.h>
#include <sub/memmap.h>
static M16 stage,last_tick,command,token,seed,expected_ret,done,result;
void memory_sub_tick(M16 now,M16 hz) {
    (void)hz;
    if(now==last_tick)return;
    last_tick=now;
    M16 mode=*ga_reg_memmode;
    if(!stage) {
        token=*ga_reg_comcmd0;if(!token)return;
        command=*ga_reg_comcmd1;seed=*ga_reg_comcmd2;expected_ret=*ga_reg_comcmd3;
        if(token!=*ga_reg_comcmd0)return;
        done=0;result=1;
        if(command<1 || command>5 || expected_ret>1){result=3;stage=4;}
        else if(command==MM_ENTER1) {
            if((mode&7u)!=2u){result=4;stage=4;}
            else {*(volatile unsigned char *)(GA_REG_MEMMODE+1)=(unsigned char)((mode&~7u)|4u);stage=1;}
        } else if(command==MM_EXIT2) {
            if(!(mode&4u) || (mode&1u)!=expected_ret){result=4;stage=4;}
            else {*(volatile unsigned char *)(GA_REG_MEMMODE+1)=(unsigned char)(mode&~7u);stage=1;}
        } else if(command==MM_SWAP1) {
            if((mode&6u)!=6u || (mode&1u)!=expected_ret){result=4;stage=4;}
            else {*(volatile unsigned char *)(GA_REG_MEMMODE+1)=(unsigned char)(mode^1u);stage=3;}
        } else stage=1;
    }
    mode=*ga_reg_memmode;
    if(stage==1) {
        M16 one=(command==MM_ENTER1 || command==MM_FILL1);
        if(one) {
            if(!(mode&4u)){result=4;stage=4;return;}
            if(command==MM_ENTER1 && (mode&1u)){result=4;stage=4;return;}
            if(command==MM_FILL1 && (mode&1u)!=expected_ret){result=4;stage=4;return;}
        } else {
            if(command==MM_EXIT2 && (mode&7u)==1u)return;
            if((mode&7u)!=2u){result=4;stage=4;return;}
        }
        volatile M16 *buffer=(volatile M16 *)(one?WORD_RAM_1M:WORD_RAM_2M);
        memory_fill(buffer,seed,token,done,16);done+=16;*ga_reg_comstat2=done;
        if(done!=M_WORDS)return;
        __asm__ volatile("" ::: "memory");
        if(one)stage=4;
        else {*(volatile unsigned char *)(GA_REG_MEMMODE+1)|=1u;stage=2;}
    }
    if(stage==2) {if((*ga_reg_memmode&7u)!=1u)return;stage=4;}
    if(stage==3) {
        if((*ga_reg_memmode&7u)!=(4u|(expected_ret^1u)))return;
        stage=4;
    }
    if(stage==4) {
        *ga_reg_comstat1=result;*ga_reg_comstat5=seed;*ga_reg_comstat6=token;
        __asm__ volatile("" ::: "memory");*ga_reg_comstat0=token;stage=5;
    }
    if(stage==5 && !*ga_reg_comcmd0) {
        *ga_reg_comstat2=0;__asm__ volatile("" ::: "memory");*ga_reg_comstat0=0;stage=0;
    }
}

#include "shared.h"
#include "stream.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/memmap.h>
#include <system.h>
extern void sp_fatal(void);
volatile u32 sub_magic=0x2468ACE0;
volatile u32 sub_zero[8];
void main(void) {
    u16 good=sub_magic==0x2468ACE0;
    for(u16 i=0;i<8;i++)if(sub_zero[i])good=0;
    *ga_reg_comstat4=good?1:0xE001;
    /* Direct registers only: never opt into the Main BIOS cache handshake. */
    *ga_reg_comflags_sub=0;
    enable_interrupts();
    /* Bootstrap wire format belongs to project startup, before System Lab takes over. */
    while(*ga_reg_comcmd0!=CMD_LOAD_FILE);
    load_file(CDROM_LOAD_CDC,"IPX.MMD;1",(u8 *)WORD_RAM_2M);
    u16 result=access_op_result;
    if(result!=CDROM_RESULT_OK)sp_fatal();
    grant_2m();
    *ga_reg_comstat1=result;
    __asm__ volatile("" ::: "memory");*ga_reg_comstat0=CMD_LOAD_FILE;
    while(*ga_reg_comcmd0);
    *ga_reg_comstat0=0;
#if VIDEO == PAL
    u16 hz=50;
#else
    u16 hz=60;
#endif
    for(;;)stream_sub_tick(*ga_reg_comstat3,hz);
}

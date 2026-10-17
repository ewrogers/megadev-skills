#include "shared.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/memmap.h>
#include <system.h>
extern void sp_fatal(void);
volatile u32 sub_magic=0x2468ACE0;
volatile u32 sub_zero[8];
static char const * const files[]={"IPX.MMD;1","ROOM.MMD;1","GARDEN.MDA;1","CAVERN.MDA;1"};
void main(void) {
    u16 good=sub_magic==0x2468ACE0;
    for(u16 i=0;i<8;i++)if(sub_zero[i])good=0;
    *ga_reg_comstat4=good?1:0xE001;
    enable_interrupts();
    for(;;) {
        u16 command=*ga_reg_comcmd0;
        if(!command || command!=*ga_reg_comcmd0)continue;
        u16 argument=*ga_reg_comcmd1,result=0;
        if(command==CMD_LOAD_FILE && argument<4) {
            load_file(CDROM_LOAD_CDC,files[argument],(u8 *)WORD_RAM_2M);
            result=access_op_result;
            grant_2m();
        } else if(command==CMD_PING)result=argument;
        else result=0xFFFF;
        *ga_reg_comstat1=result;
        __asm__ volatile("" ::: "memory");
        *ga_reg_comstat0=command;
        while(*ga_reg_comcmd0);
        *ga_reg_comstat0=0;
    }
}

#include "shared.h"
#include "probe.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
#include <sub/memmap.h>
#include <system.h>
extern void sp_fatal(void);
volatile u32 sub_magic=0x2468ACE0;
volatile u32 sub_zero[8];
static volatile u16 local_probe[HW_LOCAL_WORDS];
static char const * const files[]={"IPX.MMD;1","ROOM.MMD;1"};
void main(void) {
    u16 good=sub_magic==0x2468ACE0;
    for(u16 i=0;i<8;i++)if(sub_zero[i])good=0;
    *ga_reg_comstat4=good?1:0xE001;
    enable_interrupts();
    for(;;) {
        u16 command=*ga_reg_comcmd0;
        if(!command || command!=*ga_reg_comcmd0)continue;
        u16 argument=*ga_reg_comcmd1,result=0;
        if(command==CMD_LOAD_FILE && argument<2) {
            load_file(CDROM_LOAD_CDC,files[argument],(u8 *)WORD_RAM_2M);
            result=access_op_result;
            grant_2m();
        } else if(command==CMD_PING)result=argument;
        else if(command==CMD_WORD_CHECK) {
            /* Do not touch Word RAM if the grant or mode is wrong. */
            if((*ga_reg_memmode & 5u)!=0)result=0xE002;
            else {
                volatile u16 *buffer=(volatile u16 *)WORD_RAM_2M;
                result=hw_exercise(local_probe,HW_LOCAL_WORDS)?1:0;
                if(hw_verify(buffer,HW_WORD_COUNT,argument))result|=2;
                hw_fill(buffer,HW_WORD_COUNT,(u16)(argument^HW_RETURN_XOR));
                __asm__ volatile("" ::: "memory");
                grant_2m();
            }
        }
        else result=0xFFFF;
        *ga_reg_comstat1=result;
        __asm__ volatile("" ::: "memory");
        *ga_reg_comstat0=command;
        while(*ga_reg_comcmd0);
        *ga_reg_comstat0=0;
    }
}

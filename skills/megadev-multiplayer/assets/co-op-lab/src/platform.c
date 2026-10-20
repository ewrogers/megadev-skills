#include "platform.h"
#include <main/io.def.h>
#include <main/z80.def.h>
#include <system.h>
/* I/O callbacks are project-owned; Megadev supplies the actual register names. */
static InU8 read_port(void *context,InU16 port) {
    (void)context;
    return *(volatile unsigned char *)(port?IO_DATA2:IO_DATA1);
}
static void write_port(void *context,InU16 port,InU16 control,InU8 value) {
    (void)context;
    unsigned int address=control?(port?IO_CTRL2:IO_CTRL1):(port?IO_DATA2:IO_DATA1);
    *(volatile unsigned char *)address=value;
}
static void settle(void *context) {
    (void)context;
    __asm__ volatile("nop\n\tnop\n\tnop\n\tnop" ::: "memory");
}
static InIo const io={0,read_port,write_port,settle};
InU16 input_bus_fault;
void input_poll(InSample samples[8]) {
    volatile unsigned short *bus=(volatile unsigned short *)Z80_REG_BUSREQ;
    /* Single foreground I/O owner. Preserve a grant that was already held. */
    InU16 previous_sr=input_lock();
    unsigned short acquired=(*bus&Z80_BUS_REQUEST)!=0;
    if(acquired) {
        *bus=Z80_BUS_REQUEST;
        unsigned short wait=512;
        while((*bus&Z80_BUS_REQUEST) && wait)wait--;
        if(!wait) {
            *bus=Z80_BUS_RELEASE;input_bus_fault++;
            for(InU16 i=0;i<8;i++)in_clear(&samples[i],IN_BUS_BUSY);
            input_unlock(previous_sr);return;
        }
    }
#if INPUT_EA_MODE
    in_poll_ea(&io,samples);
#else
    in_poll_port(&io,0,samples);in_poll_port(&io,1,samples+4);
#endif
    if(acquired)*bus=Z80_BUS_RELEASE;
    input_unlock(previous_sr);
}

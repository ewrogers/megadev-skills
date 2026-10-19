#include "lab.h"
#include <main/bios.h>
#include <main/io.def.h>
#include <system.h>
volatile u32 room_magic=0xA1B2C3D4;
volatile u32 room_zero[8];
void main(void) {
    enable_interrupts();room_runs++;
    lab_screen("DISC MODULE");
    lab_line("YOU ARE RUNNING CODE IN WORD RAM",3,7);
    lab_line("INITIALIZED DATA",3,10);lab_text(room_magic==0xA1B2C3D4?"PASS":"FAIL",25,10,room_magic==0xA1B2C3D4?2:0);
    u16 good=1;for(u16 i=0;i<8;i++)if(room_zero[i])good=0;
    room_pass=good && room_magic==0xA1B2C3D4;
    lab_line("ZEROED RAM",3,12);lab_text(good?"PASS":"FAIL",25,12,good?2:0);
    lab_line("RESIDENT MAIN TICK",3,15);
    lab_line("A / START RETURN TO HARDWARE LAB",3,23);
    lab_line("WORD RAM STAYS MAIN-OWNED HERE",3,25);
    lab_present();
    /* Visible proof that each entry receives initialized data and cleared BSS. */
    room_magic=0;room_zero[0]=1;
#ifdef HW_AUTOTEST
    u16 until=(u16)(main_ticks+60);
#endif
    do {lab_wait();lab_usb_poll();if(!(main_ticks&15)){lab_hex((u16)main_ticks,28,15);lab_heartbeat((u16)main_ticks);}}
    #ifdef HW_AUTOTEST
    while((u16)(main_ticks-until)>=0x8000);
#else
    while(!(bios_joy1_hit&(PAD_A|PAD_START)));
#endif
}

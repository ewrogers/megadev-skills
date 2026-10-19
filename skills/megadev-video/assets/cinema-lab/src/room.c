/* Link/startup regression fixture; not loaded by the interactive streaming demo. */
#include "lab.h"
#include <main/bios.h>
#include <main/io.def.h>
#include <system.h>
volatile u32 room_magic=0xA1B2C3D4;
volatile u32 room_zero[8];
void main(void) {
    enable_interrupts();room_runs++;
    lab_screen("MODULE LOADED FROM DISC");
    lab_line("YOU ARE RUNNING CODE IN WORD RAM",3,7);
    lab_line("INITIALIZED DATA",3,10);lab_line(room_magic==0xA1B2C3D4?"PASS":"FAIL",28,10);
    u16 good=1;for(u16 i=0;i<8;i++)if(room_zero[i])good=0;
    lab_line("ZEROED RAM",3,12);lab_line(good?"PASS":"FAIL",28,12);
    lab_line("RESIDENT MAIN TICK",3,15);
    lab_line("A / START RETURN TO CINEMA LAB",3,23);
    lab_line("WORD RAM STAYS MAIN-OWNED HERE",3,25);
    /* Visible proof that each entry receives initialized data and cleared BSS. */
    room_magic=0;room_zero[0]=1;
    do {lab_wait();if(!(main_ticks&7))lab_hex(main_ticks,28,15);}
    while(!(bios_joy1_hit&(PAD_A|PAD_START)));
}

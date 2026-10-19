#include "audio.h"
#include <main/gate_arr.h>
A16 audio_pending,audio_rejected,audio_link_fault;
static A16 serial=1,age;
int audio_request(A16 op,A16 arg) {
    if(audio_pending || *ga_reg_comcmd4 || *ga_reg_comstat7){audio_rejected++;return 0;}
    if(op<AU_PLAY || op>AU_SILENCE || (op==AU_PLAY && (arg<2 || arg>3)) ||
       ((op==AU_EFFECT || op==AU_PAN) && arg>2) ||
       (op==AU_SPEAK && (arg<1 || arg>2)) || (op==AU_SILENCE && arg))return 0;
    serial++;if(serial<2)serial=2;
    *ga_reg_comcmd5=op;*ga_reg_comcmd6=arg;
    __asm__ volatile("" ::: "memory");*ga_reg_comcmd4=serial;audio_pending=1;age=0;return 1;
}
void audio_main_tick(A16 elapsed) {
    if(!audio_pending || audio_pending==3)return;
    if(elapsed>=9000u-age){audio_pending=3;audio_link_fault=1;return;}
    age+=elapsed;
    if(audio_pending==1 && *ga_reg_comstat7==serial){*ga_reg_comcmd4=0;audio_pending=2;}
    if(audio_pending==2 && !*ga_reg_comstat7)audio_pending=0;
}

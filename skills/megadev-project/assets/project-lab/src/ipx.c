#include "lab.h"
#include "shared.h"
#include "model.h"
#include <main/bios.h>
#include <sub/cdrom.def.h>
#include <main/gate_arr.h>
#include <main/io.def.h>
#include <main/mmd.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
static LabCursor cursor;
static LabJob job;
static u16 nonce;
void vblank_user(void){main_ticks++;}
void lab_line(char const *s,u16 x,u16 y) {
    char line[41];u16 n=0;
    while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
    lab_print(line,to_vdp_addr(VDP_PLANE_POS(x,y,Width64)+BIOS_VDP_DEFAULT_PLANEA)|VRAM_W);
}
void lab_hex(u16 value,u16 x,u16 y) {
    static char const hex[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=hex[(value>>(12-i*4))&15];
    s[4]=0;lab_line(s,x,y);
}
void lab_screen(char const *title) {
    lab_display_off();lab_clear();
    lab_line("+------------------------------------+",1,1);
    lab_line(title,3,3);
    lab_line("+------------------------------------+",1,5);
    lab_line("+------------------------------------+",1,26);
    lab_display_on();
}
static void home(void) {
    lab_screen("PROJECT LAB");
    lab_line("MAIN C STARTUP",3,7);lab_line(main_magic==0x12345678?"PASS":"FAIL",28,7);
    u16 good=1;for(u16 i=0;i<8;i++)if(main_zero[i])good=0;
    lab_line("MAIN ZEROED RAM",3,8);lab_line(good?"PASS":"FAIL",28,8);
    lab_line("SUB C STARTUP",3,9);lab_line(*ga_reg_comstat4==1?"PASS":"FAIL",28,9);
    lab_line("DISC MODULE LOADS",3,11);lab_hex(room_runs,28,11);
    lab_line("MAIN TICKS",3,13);lab_line("SUB TICKS",23,13);
    lab_line("PING REPLY",3,15);
    lab_line("A LOAD MODULE   B PING SUB CPU",3,22);
    lab_line("D-PAD MOVE      START HOLD MARKER",3,23);
#if REGION == EU
    lab_line("EUROPE BUILD  PAL",3,25);
#elif REGION == JP
    lab_line("JAPAN BUILD  NTSC",3,25);
#else
    lab_line("USA BUILD  NTSC",3,25);
#endif
}
void main(void) {
    *bios_vblank_user=vblank_user;
    enable_interrupts();lab_display_off();lab_font();
    bios_palette[0]=0x0200;
    for(u16 i=1;i<16;i++)bios_palette[i]=0x0EEE;
    bios_vdp_update_flags|=BIOS_MASK_COPY_PALETTE;
    cursor.x=20;cursor.y=18;home();
    for(;;) {
        lab_wait();
        u16 x=cursor.x,y=cursor.y;
        lab_cursor_step(&cursor,bios_joy1_hold);
        if(x!=cursor.x || y!=cursor.y)lab_line(" ",x,y);
        lab_line(cursor.paused?"X":"+",cursor.x,cursor.y);
        if(!(main_ticks&7)) {
            lab_hex(main_ticks,14,13);lab_hex(*ga_reg_comstat3,33,13);
            lab_hex(*ga_reg_comstat1,14,15);
        }
        if(job.phase==LAB_FAILED)continue;
        u16 pressed=bios_joy1_hit;
        if(job.phase==LAB_IDLE && !*ga_reg_comstat0 && (pressed&(PAD_A|PAD_B))) {
            u16 cmd=pressed&PAD_A?CMD_LOAD_FILE:CMD_PING;
            if(lab_request(&job,cmd,180)) {
                if(cmd==CMD_LOAD_FILE)grant_2m();
                *ga_reg_comcmd1=cmd==CMD_LOAD_FILE?FILE_ROOM_MMD:++nonce;
                __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=cmd;
                lab_line(cmd==CMD_LOAD_FILE?"LOADING FROM DISC...":"PING REQUEST...     ",3,24);
            }
        }
        u16 command=job.command;
        int event=lab_poll(&job,*ga_reg_comstat0,*ga_reg_comstat1);
        if(event==1)*ga_reg_comcmd0=0;
        if(event<0) {lab_line("LINK TIMEOUT  RESET DISC TO RETRY",3,24);continue;}
        if(event==2) {
            if(command==CMD_LOAD_FILE) {
                if(job.result!=CDROM_RESULT_OK) {lab_line("DISC READ FAILED  RESET TO RETRY",3,24);job.phase=LAB_FAILED;continue;}
                void (*entry)(void)=(void (*)(void))init_mmd();entry();home();
            } else lab_line("PING RECEIVED                   ",3,24);
        }
    }
}

#include "lab.h"
#include "shared.h"
#include "probe.h"
#include <main/memmap.h>
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
volatile u16 main_ticks,room_runs,room_pass;
static LabJob job;
static u16 ping_seed;
#ifdef HW_AUTOTEST
static u16 auto_step,auto_due=90,auto_failed,auto_finished;
#endif
static u16 nonce, main_probe_ok, sub_probe_ok, sub_received_ok, main_received_ok;
static u16 probe_runs, probe_seed, pal_hardware;
static volatile u16 local_probe[HW_LOCAL_WORDS];
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
static void result_line(char const *label,u16 good,u16 y) {
    lab_line(label,3,y);lab_line(probe_runs?(good?"PASS":"FAIL"):"----",30,y);
}
static void home(void) {
    lab_screen("HARDWARE LAB");
    u16 good=main_magic==0x12345678;for(u16 i=0;i<8;i++)if(main_zero[i])good=0;
    lab_line("MAIN C STARTUP",3,7);lab_line(good?"PASS":"FAIL",30,7);
    lab_line("SUB C STARTUP",3,8);lab_line(*ga_reg_comstat4==1?"PASS":"FAIL",30,8);
    result_line("MAIN SCRATCH RAM",main_probe_ok,10);
    result_line("SUB SCRATCH RAM",sub_probe_ok,11);
    result_line("MAIN TO SUB WORD RAM",sub_received_ok,12);
    result_line("SUB TO MAIN WORD RAM",main_received_ok,13);
    lab_line("CHECKS",3,15);lab_hex(probe_runs,10,15);
    lab_line("MODE",17,15);lab_hex(*ga_reg_memmode,23,15);
    lab_line("MAIN TICK",3,17);lab_line("SUB TICK",23,17);
    lab_line("REPLY",3,18);
    lab_line("DISC RELOADS",3,19);lab_hex(room_runs,19,19);
    lab_line("A CHECK RAM   B PING   C DISC",3,22);
    lab_line("ONLY RESERVED BUFFERS ARE TESTED",3,23);
    lab_line(pal_hardware?"VDP PAL HARDWARE":"VDP NTSC HARDWARE",3,25);
}

void main(void) {
    *bios_vblank_user=vblank_user;
    enable_interrupts();lab_display_off();lab_font();
    pal_hardware=(vdp_ctrl_16 & VDPSTAT_MASK_PAL_HARDWARE)!=0;
    bios_palette[0]=0x0200;
    for(u16 i=1;i<16;i++)bios_palette[i]=0x0EEE;
    bios_vdp_update_flags|=BIOS_MASK_COPY_PALETTE;
    home();
    for(;;) {
        lab_wait();
        if(!(main_ticks&7)) {
            lab_hex(main_ticks,13,17);lab_hex(*ga_reg_comstat3,32,17);
            lab_hex(*ga_reg_comstat1,9,18);
        }
        if(job.phase==LAB_FAILED)continue;
#ifdef HW_AUTOTEST
        if(auto_step==6 && job.phase==LAB_IDLE && !auto_finished) {
            auto_finished=1;
            if(probe_runs!=3 || room_runs!=2 || !room_pass)auto_failed=1;
            lab_line(auto_failed?"AUTO TOUR FAILED":"AUTO TOUR PASS",3,20);
        }
#endif
        u16 pressed=bios_joy1_hit;
#ifdef HW_AUTOTEST
        /* A deterministic input source for emulator QA, not a replacement for controller testing. */
        pressed=0;
        if(auto_step<6 && job.phase==LAB_IDLE && (u16)(main_ticks-auto_due)<0x8000) {
            static u16 const tour[]={PAD_A,PAD_A,PAD_B,PAD_C,PAD_C,PAD_A};
            pressed=tour[auto_step++];auto_due=(u16)(main_ticks+90);
        }
#endif
        if(job.phase==LAB_IDLE && !*ga_reg_comstat0 && (pressed&(PAD_A|PAD_B|PAD_C))) {
            u16 cmd=pressed&PAD_A?CMD_WORD_CHECK:(pressed&PAD_B?CMD_PING:CMD_LOAD_FILE);
            if(lab_request(&job,cmd,180)) {
                if(cmd==CMD_WORD_CHECK) {
                    if((*ga_reg_memmode&5u)!=1u) {
                        lab_line("WORD RAM OWNER/MODE ERROR",3,24);job.phase=LAB_FAILED;continue;
                    }
                    main_probe_ok=hw_exercise(local_probe,HW_LOCAL_WORDS);
                    probe_seed=(u16)(++nonce ^ 0x39C6);
                    hw_fill((volatile u16 *)WORD_RAM,HW_WORD_COUNT,probe_seed);
                }
                if(cmd==CMD_LOAD_FILE || cmd==CMD_WORD_CHECK) {
                    __asm__ volatile("" ::: "memory");grant_2m();
                }
                if(cmd==CMD_PING)ping_seed=++nonce;
                *ga_reg_comcmd1=cmd==CMD_LOAD_FILE?FILE_ROOM_MMD:(cmd==CMD_WORD_CHECK?probe_seed:ping_seed);
                __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=cmd;
                lab_line(cmd==CMD_WORD_CHECK?"CHECKING OWNED BUFFERS...":(cmd==CMD_LOAD_FILE?"LOADING FROM DISC...     ":"PING REQUEST...         "),3,24);
            }
        }
        u16 command=job.command;
        int event=lab_poll(&job,*ga_reg_comstat0,*ga_reg_comstat1);
        if(event==1)*ga_reg_comcmd0=0;
        if(event<0) {lab_line("LINK TIMEOUT  RESET DISC TO RETRY",3,24);continue;}
        if(event==2) {
            if(command==CMD_LOAD_FILE) {
                if(job.result!=CDROM_RESULT_OK) {lab_line("DISC READ FAILED  RESET TO RETRY",3,24);job.phase=LAB_FAILED;continue;}
                if((*ga_reg_memmode&5u)!=1u) {lab_line("WORD RAM NOT RETURNED  RESET",3,24);job.phase=LAB_FAILED;continue;}
                void (*entry)(void)=(void (*)(void))init_mmd();entry();home();
#ifdef HW_AUTOTEST
                if(!room_pass)auto_failed=1;
#endif
            } else if(command==CMD_WORD_CHECK) {
                if((*ga_reg_memmode&5u)!=1u) {lab_line("WORD RAM NOT RETURNED  RESET",3,24);job.phase=LAB_FAILED;continue;}
                sub_probe_ok=(job.result&1)!=0;sub_received_ok=(job.result&2)!=0;
                main_received_ok=job.result<=3 && hw_verify((volatile u16 *)WORD_RAM,HW_WORD_COUNT,(u16)(probe_seed^HW_RETURN_XOR));
                probe_runs++;home();
#ifdef HW_AUTOTEST
                if(!main_probe_ok || !sub_probe_ok || !sub_received_ok || !main_received_ok)auto_failed=1;
#endif
                lab_line("BUFFER CHECK COMPLETE",3,24);
            } else {
#ifdef HW_AUTOTEST
                if(job.result!=ping_seed)auto_failed=1;
#endif
                lab_line(job.result==ping_seed?"PING RECEIVED":"PING MISMATCH",3,24);
            }
        }
    }
}

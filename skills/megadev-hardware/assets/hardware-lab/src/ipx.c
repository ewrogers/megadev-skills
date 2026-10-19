#include "lab.h"
#include "everdrive_link.h"
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
volatile u32 main_ticks;
volatile u16 room_runs,room_pass;
static HlEdLink usb;
static u16 refreshed;
static LabJob job;
static u16 ping_seed;
#ifdef HW_AUTOTEST
static u16 auto_step,auto_due=90,auto_failed,auto_finished;
#endif
static u16 nonce, main_probe_ok, sub_probe_ok, sub_received_ok, main_received_ok;
static u16 probe_runs, probe_seed, pal_hardware;
static volatile u16 local_probe[HW_LOCAL_WORDS];
void vblank_user(void){main_ticks++;}
#define ED_STATUS (*(volatile u16 *)0xa130d4)
#define ED_FIFO_STATUS (*(volatile u16 *)0xa130d2)
#define ED_FIFO_DATA (*(volatile u8 *)0xa130d1)
void lab_usb_poll(void) {
    hl_ed_tick(&usb,main_ticks);
    /* MCU also services CD. No disk setup/flush, and no guest traffic during
     * a Main/Sub job. This lab does not qualify concurrent CD/audio streaming. */
#ifndef HW_USB_LAB
    return; /* Explicit fallback for setups without CD-mode host USB. */
#endif
    if(!usb.present || job.phase!=LAB_IDLE || *ga_reg_comstat0)return;
    u16 count=ED_FIFO_STATUS&0x7ff;if(count>32)count=32;
    for(u16 i=0;i<count;i++) {
        u8 reply[HL_ED_PACKET_BYTES];
        if(hl_ed_receive(&usb,ED_FIFO_DATA,main_ticks,reply)) {
            ED_FIFO_DATA=0x2b;ED_FIFO_DATA=0xd4;ED_FIFO_DATA=0x22;ED_FIFO_DATA=0xdd;
            ED_FIFO_DATA=0;ED_FIFO_DATA=HL_ED_PACKET_BYTES;
            for(u16 n=0;n<HL_ED_PACKET_BYTES;n++)ED_FIFO_DATA=reply[n];
        }
    }
}
static void result_line(char const *label,u16 good,u16 y) {
    lab_line(label,3,y);lab_text(probe_runs?(good?"PASS":"FAIL"):"----",25,y,good?2:0);
}
static void home(void) {
    lab_screen("HARDWARE LAB");
    u16 good=main_magic==0x12345678;for(u16 i=0;i<8;i++)if(main_zero[i])good=0;
    lab_line("MAIN C STARTUP",3,7);lab_text(good?"PASS":"FAIL",25,7,good?2:0);
    lab_line("SUB C STARTUP",3,8);lab_text(*ga_reg_comstat4==1?"PASS":"FAIL",25,8,*ga_reg_comstat4==1?2:0);
    result_line("MAIN SCRATCH RAM",main_probe_ok,10);
    result_line("SUB SCRATCH RAM",sub_probe_ok,11);
    result_line("WORD RAM TO SUB",sub_received_ok,12);
    result_line("WORD RAM TO MAIN",main_received_ok,13);
    lab_line("EVERDRIVE EDIO",3,15);lab_text(usb.present?"DETECTED":"ABSENT",25,15,1);
#ifdef HW_USB_LAB
    lab_line("USB HOST CHECK",3,16);
    lab_text(usb.verified?"PASS/LIVE":(usb.ever_verified?"PASS/IDLE":"NOT RUN  "),25,16,usb.ever_verified?2:3);
#else
    lab_line("USB UPLOAD",3,16);lab_text("MENU MODE",25,16,3);
#endif
    lab_text("CHECKS     RELOADS    MAIN / SUB",3,18,3);
    lab_hex(probe_runs,3,19);lab_hex(room_runs,14,19);
    lab_line("A RAM     B PING     C DISC",3,22);
    lab_text("RESERVED BUFFERS ONLY",3,23,3);
    lab_text(pal_hardware?"VIDEO PAL":"VIDEO NTSC",3,26,3);
    lab_present();

}

void main(void) {
    *bios_vblank_user=vblank_user;
    enable_interrupts();lab_display_off();
    pal_hardware=(vdp_ctrl_16 & VDPSTAT_MASK_PAL_HARDWARE)!=0;
    const u16 first=ED_STATUS,second=ED_STATUS;
    hl_ed_init(&usb,hl_ed_signature(first,second),pal_hardware?50:60);
    lab_video_init();
    home();
    for(;;) {
        lab_wait();
        lab_usb_poll();
        if((u16)(main_ticks-refreshed)>=16) {
            refreshed=(u16)main_ticks;
            lab_hex((u16)main_ticks,25,19);lab_hex(*ga_reg_comstat3,32,19);
            #ifdef HW_USB_LAB
            lab_text(usb.verified?"PASS/LIVE":(usb.ever_verified?"PASS/IDLE":"NOT RUN  "),25,16,usb.ever_verified?2:3);
            #endif
            lab_heartbeat((u16)main_ticks);
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

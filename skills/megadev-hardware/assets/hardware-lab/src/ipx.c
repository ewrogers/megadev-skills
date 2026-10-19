#include "lab.h"
#include "everdrive_link.h"
#include "usb_input_ed.h"
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
static UsbInput remote_input;
static u16 input_page,input_phase;
static u16 drawn_x[2],drawn_y[2];
static u16 pointer_x[2]={48,48},pointer_y[2]={8,8};
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
#ifdef HW_USB_LAB
static void legacy_packet(void *context,const UiU8 *packet,UiU32 now) {
    HlEdLink *link=context;u8 reply[HL_ED_PACKET_BYTES];
    for(u16 i=0;i<HL_ED_PACKET_BYTES;i++)
        if(hl_ed_receive(link,packet[i],now,reply))ui_ed_send(reply,HL_ED_PACKET_BYTES);
}
#endif
const UiPort *lab_input_port(u16 port) {return port<2?&remote_input.port[port]:0;}
u16 lab_input_pressed(void) {return remote_input.port[0].kind==UI_PAD?remote_input.port[0].pressed:0;}
void lab_usb_poll(void) {
    UiPort physical[2]={{.kind=UI_PAD,.held=bios_joy1_hold},{.kind=UI_PAD,.held=bios_joy2_hold}};
    hl_ed_tick(&usb,main_ticks);
    /* Same FIFO owner; MCU/CD work takes precedence. Lease expiry and physical
     * sampling continue while requests are deferred by an active Main/Sub job. */
#ifdef HW_USB_LAB
    if(job.phase==LAB_IDLE && !*ga_reg_comstat0)ui_ed_poll(&remote_input,main_ticks,legacy_packet,&usb);
#endif
    ui_ed_finish(&remote_input,main_ticks,physical);
    for(u16 i=0;i<2;i++) {
        const UiPort *p=&remote_input.port[i];
        s32 x=pointer_x[i]+(p->kind==UI_MOUSE?p->dx:((p->held&PAD_RIGHT?1:0)-(p->held&PAD_LEFT?1:0)));
        s32 y=pointer_y[i]+(p->kind==UI_MOUSE?p->dy:((p->held&PAD_DOWN?1:0)-(p->held&PAD_UP?1:0)));
        pointer_x[i]=x<0?0:x>111?111:(u16)x;pointer_y[i]=y<0?0:y>15?15:(u16)y;
    }
}
static void input_open(void) {
    input_phase=0;for(u16 i=0;i<2;i++){drawn_x[i]=3+i*19;drawn_y[i]=19;}
    lab_input_screen();
    lab_text("PORT 1",3,7,1);lab_text("PORT 2",22,7,1);
    for(u16 i=0;i<2;i++) {
        u16 x=3+i*19;
        lab_text("HELD",x,10,3);lab_text("PRESSED",x,11,3);lab_text("RELEASED",x,12,3);
        lab_text("DX",x,14,3);lab_text("DY",x+8,14,3);lab_text("XY",x,15,3);
    }
    lab_text("POINTER",3,17,1);lab_text("POINTER",22,17,1);
    lab_text("START: HARDWARE LAB",3,24,1);
    lab_text("PHYSICAL PAD READY",3,25,3);lab_present();
}
static void input_update(void) {
    /* At most two existing VBlank-safe text calls per input iteration. */
    const UiPort *a=&remote_input.port[0],*b=&remote_input.port[1];
    u16 phase=input_phase++%12;
    if(phase<2) {
        const UiPort *p=phase?b:a;u16 x=3+phase*19;
        lab_text(p->remote?(p->kind==UI_MOUSE?"USB MOUSE     ":"USB PAD       "):"BIOS PAD      ",x,8,p->remote?2:3);
    }else if(phase<5) {
        u16 av=phase==2?a->held:phase==3?a->pressed:a->released;
        u16 bv=phase==2?b->held:phase==3?b->pressed:b->released;
        lab_hex(av,12,8+phase);lab_hex(bv,31,8+phase);
    }else if(phase<7) {
        const UiPort *p=phase==5?a:b;u16 x=phase==5?3:22;
        lab_hex((u16)p->dx,x+2,14);lab_hex((u16)p->dy,x+10,14);
    }else if(phase<9) {
        u16 i=phase-7,x=3+i*19;lab_hex(pointer_x[i],x+2,15);lab_hex(pointer_y[i],x+9,15);
    }else if(phase<11) {
        u16 i=phase-9;lab_text(" ",drawn_x[i],drawn_y[i],0);
        drawn_x[i]=3+i*19+pointer_x[i]/8;drawn_y[i]=19+pointer_y[i]/8;
        lab_text("+",drawn_x[i],drawn_y[i],i?1:2);
    }else {
        lab_heartbeat((u16)main_ticks);
        lab_text(a->remote?"USB P1 ACTIVE / PAD PAUSED       ":
                 b->remote?"USB P2 ACTIVE / P1 READY         ":
                           "PHYSICAL PAD READY              ",3,25,3);
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
    lab_text("SCRATCH BUFFERS ONLY",3,23,3);
    lab_text("START: INPUT LAB",3,25,1);
    lab_text(pal_hardware?"VIDEO PAL":"VIDEO NTSC",3,26,3);
    lab_present();

}

void main(void) {
    *bios_vblank_user=vblank_user;
    enable_interrupts();lab_display_off();
    pal_hardware=(vdp_ctrl_16 & VDPSTAT_MASK_PAL_HARDWARE)!=0;
    hl_ed_init(&usb,ui_ed_detect(),pal_hardware?50:60);
    ui_init(&remote_input,
#ifdef HW_USB_LAB
            usb.present,
#else
            0,
#endif
            pal_hardware?50:60);
    lab_video_init();
    home();
    for(;;) {
        lab_wait();
        lab_usb_poll();
        if(job.phase==LAB_IDLE && (lab_input_pressed()&PAD_START)) {
            input_page^=1;if(input_page)input_open();else home();
        }
        if(input_page) {input_update();continue;}
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
        u16 pressed=lab_input_pressed();
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

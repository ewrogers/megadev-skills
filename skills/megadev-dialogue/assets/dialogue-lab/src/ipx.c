#include "lab.h"
#include "paint.h"
#include "dialogue.h"
#include "speech.h"
#include "audio.h"
#include <main/bios.h>
#include <main/io.def.h>
#include <main/vdp.h>
#include <system.h>
#include <build.def.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
Dialogue dialogue;
Speech speech;
UiCanvas desired;
UiPainter painter;
UiBatch batch;
volatile u16 published,consumed,input_hits,input_held,video_fault,committed_pages;
static u16 hz;
extern u8 const res_fonts[],res_heading[],res_panels[],res_icons[],res_portrait[],res_backdrop[];
extern u16 const res_palette[];
void lab_upload_work(void const *data,u32 address,u32 words);
void ui_install(void);
static void reg(u16 index,u16 value) {
    u16 command=(u16)(0x8000u|(index<<8)|value);
    bios_vdp_regs[index]=command;vdp_ctrl_16=command;
}
void vblank_user(void) {
    main_ticks++;input_hits|=bios_joy1_hit;input_held=bios_joy1_hold;
    if(!published)return;
    if(!ui_batch_valid(&batch)){video_fault=1;return;}
    u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
    /* Defer an immutable request if the callback misses its early blank window. */
    if(!(vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) || vertical<224 || vertical>=231)return;
    u16 base=batch.page?UI_PAGE1:UI_PAGE0;
    for(u16 i=0;i<batch.count;i++) {
        vdp_ctrl_32=to_vdp_addr(base+batch.rows[i]*128u)|VRAM_W;
        for(u16 x=0;x<UI_COLS;x++)vdp_data_16=batch.words[i][x];
    }
    if(batch.flip)reg(2,base>>10);
    consumed=batch.epoch;
    __asm__ volatile("" ::: "memory");published=0;
}
static void initialize(void) {
    lab_display_off();
    reg(2,UI_PAGE0>>10);reg(3,0x9000>>10);reg(4,UI_PLANEB>>13);
    reg(5,UI_SAT>>9);reg(7,0);reg(11,0);reg(13,UI_HSCROLL>>10);reg(15,2);reg(16,1);reg(17,0);reg(18,0);
    lab_upload_work(res_fonts,to_vdp_addr(UI_FONT*32u)|VRAM_W,192*16);
    lab_upload_work(res_heading,to_vdp_addr(UI_HEADING*32u)|VRAM_W,144*16);
    lab_upload_work(res_panels,to_vdp_addr(UI_PANEL*32u)|VRAM_W,13*16);
    lab_upload_work(res_icons,to_vdp_addr(UI_ICONS*32u)|VRAM_W,32*16);
    lab_upload_work(res_portrait,to_vdp_addr(UI_PORTRAIT*32u)|VRAM_W,216*16);
    lab_upload_work(res_backdrop,to_vdp_addr(UI_BACKDROP*32u)|VRAM_W,4*16);
    vdp_ctrl_32=CRAM_W;
    for(u16 i=0;i<64;i++){bios_palette[i]=res_palette[i];vdp_data_16=res_palette[i];}
    for(u16 page=0;page<2;page++) {
        vdp_ctrl_32=to_vdp_addr(page?UI_PAGE1:UI_PAGE0)|VRAM_W;
        for(u16 i=0;i<2048;i++)vdp_data_16=0;
    }
    vdp_ctrl_32=to_vdp_addr(UI_PLANEB)|VRAM_W;
    for(u16 y=0;y<32;y++)for(u16 x=0;x<64;x++)vdp_data_16=(u16)(UI_BACKDROP+2+((x+y)&1u));
    vdp_ctrl_32=to_vdp_addr(UI_SAT)|VRAM_W;for(u16 i=0;i<4;i++)vdp_data_16=0;
    vdp_ctrl_32=to_vdp_addr(UI_HSCROLL)|VRAM_W;vdp_data_16=0;vdp_data_16=0;
    vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
    d_init(&dialogue);ui_painter_init(&painter);
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    ui_install();enable_interrupts();lab_display_on();
}
#ifdef DIALOGUE_AUTOTEST
static u16 tour_step,tour_age;
void dialogue_tour(u16 elapsed) {
    tour_age+=elapsed;
    if(tour_age<150 || painter.building || painter.waiting || published)return;
    tour_age=0;
    if(dialogue.mode==D_HUB) {
        if(tour_step>=2)return;
        d_start(&dialogue,tour_step++);
    } else if(dialogue.mode==D_TALK && dialogue.ready)d_step(&dialogue,UI_A,0,0);
    else if(dialogue.mode==D_DONE)d_step(&dialogue,UI_A,0,0);
}
#endif
void dialogue_audio_tick(u16 elapsed,u16 pressed) {
    audio_main_tick(elapsed);
    speech_want(&speech,d_voice(&dialogue),dialogue.generation);
    if(speech.inflight && !audio_pending)speech_complete(&speech,0);
    if(audio_pending==3){if(speech.inflight)speech_complete(&speech,1);else speech.fault=1;}
    if(!audio_pending && !speech.fault) {
        u16 op=speech_next(&speech);
        if(op){if(audio_request(op,op==SPEECH_PLAY?speech.wanted:0))speech_sent(&speech,op);}
        else if(pressed&UI_A)audio_request(AU_EFFECT,1);
    }
}
u16 dialogue_controls(u16 pressed) {
    /* Reuse generation one only after old renderer/audio requests have retired. */
    if(dialogue.generation==65534u && (published || painter.waiting || painter.building || speech.inflight || audio_pending))
        return (u16)(pressed&~(UI_A|UI_B|UI_START));
    return pressed;
}
void main(void) {
    initialize();u16 last=main_ticks;
    for(;;) {
        lab_wait();
        disable_interrupts();
        u16 ticks=main_ticks,pressed=input_hits,held=input_held;input_hits=0;
        enable_interrupts();
        if(!published && painter.waiting && consumed==batch.epoch) {
            if(!ui_ack(&painter,&batch))video_fault=1;
            else if(!painter.building){d_present(&dialogue,painter.screen);committed_pages|=(u16)(1u<<dialogue.mode);}
        }
        u16 frames=(u16)(ticks-last);last=ticks;if(frames>4)frames=4;
        u16 elapsed=(u16)(frames*(hz==50?6u:5u));
        d_step(&dialogue,dialogue_controls(pressed),held,elapsed);
        dialogue_audio_tick(elapsed,pressed);
#ifdef DIALOGUE_AUTOTEST
        dialogue_tour(elapsed);
#endif
        if(!video_fault && !published && !painter.waiting) {
            if(!d_draw(&desired,&dialogue,hz)){video_fault=1;continue;}
            if(speech.fault)ui_text(&desired,31,1,8,0,2,"NO LINK");
            if(ui_prepare(&painter,&desired,dialogue.generation,&batch)) {
                __asm__ volatile("" ::: "memory");published=1;
            }
        }
    }
}

#include "lab.h"
#include "shared.h"
#include "model.h"
#include "asset.h"
#include "gfx.h"
#include "../build/asset_sizes.h"
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/io.def.h>
#include <main/mmd.h>
#include <main/vdp.h>
#include <sub/cdrom.def.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_ship[],res_font[];
#define SHIP_TILE 0x120
#define SKY_TILE 0x180
#define PATTERN_END 0x500
#define LAB_SPRITE_CAP 24
/* Inactive frame is private to Main; publish its index as one atomic word. */
typedef struct {GfxList sprites;u16 palette[64],scroll[2],hud[80];} Frame;
static Frame frames[2];
static GfxState state;
static u16 base_palette[64],stage,sky_tiles,hz,ready_art,tour_pass;
#ifdef GRAPHICS_AUTOTEST
static u16 tour_seen;
#endif
static volatile u16 published,submitted,upload_high,upload_fault;
static LabJob job;
static char const * const effect_names[]={"NORMAL","CYCLE","FADE","FLASH"};
void lab_line(char const *s,u16 x,u16 y) {
    char line[41];u16 n=0;
    if(x>=40 || y>=4)return;
    while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
    lab_print(line,to_vdp_addr(VDP_PLANE_POS(x,y,Width64)+BIOS_VDP_DEFAULT_WINDOW)|VRAM_W);
}
void lab_hex(u16 value,u16 x,u16 y) {
    static char const hex[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=hex[(value>>(12-i*4))&15];
    s[4]=0;lab_line(s,x,y);
}
void lab_screen(char const *title){lab_line(title,1,0);}
/* Called by the existing BIOS VBlank handler. No loads, CRC, waits or simulation. */
void vblank_user(void) {
    main_ticks++;
    u16 index=published;if(!index)return;
    Frame const *f=&frames[index-1];
    u16 count=f->sprites.count,bytes=gfx_upload_bytes(count,80);
    if(count>LAB_SPRITE_CAP || !bytes || bytes>GFX_UPLOAD_MAX){upload_fault=1;published=0;return;}
    if(bytes>upload_high)upload_high=bytes;
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    u16 n=count?count:1;
    for(u16 i=0;i<n;i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sprites.sat[i][w];
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL)|VRAM_W;
    vdp_data_16=f->scroll[0];vdp_data_16=f->scroll[1];
    vdp_ctrl_32=CRAM_W;
    for(u16 i=0;i<64;i++){bios_palette[i]=f->palette[i];vdp_data_16=f->palette[i];}
    for(u16 row=0;row<2;row++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+(row+2)*128)|VRAM_W;
        for(u16 x=0;x<40;x++)vdp_data_16=f->hud[row*40+x];
    }
    submitted++;published=0;
}
static void reg(u16 index,u16 value) {
    u16 word=(u16)(0x8000u|(index<<8)|value);
    bios_vdp_regs[index]=word;vdp_ctrl_16=word;
}
static void palette(volatile u8 const *data,u16 line) {
    for(u16 i=0;i<16;i++)base_palette[line*16+i]=asset_word(data+32+i*2);
}
static u16 load_picture(u16 layer,volatile u8 const *data,u32 length) {
    AssetView view;
    if(!asset_open(data,length,&view) || view.kind || view.width!=64 || view.height!=24)return 0;
    u16 base=layer?(u16)(SKY_TILE+sky_tiles):SKY_TILE;
    if(view.tiles>PATTERN_END-base)return 0;
    if(!layer)sky_tiles=view.tiles;
    disable_interrupts();
    lab_upload_word((void const *)(data+64),to_vdp_addr(base*32)|VRAM_W,view.tile_bytes/2);
    for(u16 y=0;y<24;y++) {
        u16 address=(u16)((layer?BIOS_VDP_DEFAULT_PLANEA:BIOS_VDP_DEFAULT_PLANEB)+(y+4)*128);
        vdp_ctrl_32=to_vdp_addr(address)|VRAM_W;
        for(u16 x=0;x<64;x++) {
            u16 cell=asset_word(data+view.map_offset+(y*64u+x)*2u);
            vdp_data_16=(u16)((cell&0x1800u)|((cell&0x7FFu)+base)|((layer?3:2)<<13)|(layer?0x8000:0));
        }
    }
    enable_interrupts();palette(data,layer?3:2);return 1;
}
static u16 shared(void) {
    AssetView ship;
    if(!asset_open(res_ship,ASSET_SHIP_BYTES,&ship) || ship.kind!=1 || ship.width!=4 || ship.height!=3 || ship.entries!=2 || ship.tiles!=24)return 0;
    palette(res_ship,1);
    lab_upload_work(res_ship+64,to_vdp_addr(SHIP_TILE*32)|VRAM_W,ship.tile_bytes/2);
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    base_palette[1]=0x0EEE;base_palette[2]=0x0220;
    for(u16 i=0;i<64;i++)bios_palette[i]=base_palette[i];
    bios_vdp_update_flags=0; /* This renderer owns CRAM, never race a BIOS fade. */
    vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++)vdp_data_16=base_palette[i];
    /* Full-plane scrolling; Window covers top four rows, with opaque font tiles. */
    reg(11,0);reg(17,0);reg(18,4);reg(7,0x20);
    vdp_ctrl_32=VSRAM_W;vdp_data_16=0;vdp_data_16=0;
    for(u16 y=0;y<4;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW+y*128)|VRAM_W;
        for(u16 x=0;x<64;x++)vdp_data_16=0x8020;
    }
    return 1;
}
static void text(Frame *f,u16 x,u16 row,char const *s) {
    while(*s && x<40){f->hud[row*40+x]=(u16)(0x8000|(u8)*s);s++;x++;}
}
static void number(Frame *f,u16 x,u16 row,u16 value) {
    char digits[5];u16 place=1000;
    for(u16 i=0;i<4;i++){digits[i]='0';while(value>=place && digits[i]<'9'){value-=place;digits[i]++;}place=i==0?100:(i==1?10:1);}
    digits[4]=0;text(f,x,row,digits);
}
static void prepare_frame(void) {
    Frame *f=&frames[stage];gfx_begin(&f->sprites,LAB_SPRITE_CAP);
    u16 frame=gfx_frame(state.tick,6,2);
    /* Admit the controlled ship first, then less important scenery actors. */
    gfx_sprite(&f->sprites,(s16)(state.x-state.camera),(s16)state.y,32,24,(u16)(SHIP_TILE+frame*12),1,state.front,state.flip);
    for(u16 i=0;i<5;i++) {
        s16 x=(s16)((u16)(i*176u+80u)-state.camera);
        s16 y=(s16)(64+i*24u+((state.tick>>4)&3u));
        gfx_sprite(&f->sprites,x,y,32,24,(u16)(SHIP_TILE+((frame+i)&1u)*12),1,0,i&1u);
    }
    f->scroll[0]=gfx_scroll(state.camera,1);
    f->scroll[1]=gfx_scroll(state.camera,state.parallax?4:1);
    gfx_palette(base_palette,f->palette,state.tick,state.effect);
    for(u16 i=0;i<80;i++)f->hud[i]=0x8020;
    text(f,1,0,"A ");text(f,3,0,effect_names[state.effect]);
    text(f,10,0,state.parallax?"B PARALLAX ON":"B PARALLAX OFF");
    text(f,25,0,state.front?"C IN FRONT":"C BEHIND");
    if(tour_pass)text(f,1,1,"AUTO PASS");else text(f,1,1,state.paused?"PAUSED":"FLYING");
    text(f,11,1,"SPR");number(f,15,1,f->sprites.count);
    text(f,21,1,"UPLOAD");number(f,28,1,gfx_upload_bytes(f->sprites.count,80));
    text(f,33,1,"B");text(f,36,1,hz==50?"PAL":"NTSC");
    /* Word publication follows all buffer writes, with no competing VDP owner. */
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
void main(void) {
    *bios_vblank_user=vblank_user;enable_interrupts();lab_display_off();lab_clear();
    disable_interrupts();u16 good=shared();enable_interrupts();
    lab_line("GRAPHICS LAB / LANTERN QUAY",1,0);
#ifdef GRAPHICS_AUTOTEST
    lab_line("AUTOMATIC TOUR - ENDS PAUSED",1,1);
#else
    lab_line("D-PAD FLY   START PAUSE",1,1);
#endif
    if(!good){lab_line("SHARED ART FAILED",1,2);for(;;)lab_wait();}
    gfx_reset(&state);
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    u16 loading=0,pending=1;
    for(;;) {
        if(ready_art) {
            u16 hit=bios_joy1_hit;
            if(hit&PAD_A)state.effect=(state.effect+1)&3;
            if(hit&PAD_B)state.parallax^=1;
            if(hit&PAD_C)state.front^=1;
            if(hit&PAD_START)state.paused^=1;
            u16 held=bios_joy1_hold;
#ifdef GRAPHICS_AUTOTEST
            held=state.tick<480?PAD_RIGHT:PAD_LEFT;
            if(state.tick<600) {
                state.effect=(state.tick/120)&3;state.front=(state.tick/90)&1;
                state.parallax=!(state.tick>=300 && state.tick<360);tour_seen|=(u16)(1u<<state.effect);
            } else {
                tour_pass=tour_seen==15 && submitted>=400 && !upload_fault;
                state.paused=1;state.effect=0;state.parallax=1;
            }
#endif
            gfx_step(&state,held,hz);prepare_frame();
        } else if(pending && job.phase==LAB_IDLE && !*ga_reg_comstat0) {
            if((*ga_reg_memmode&5u)!=1u){lab_line("WORD RAM OWNER ERROR",1,2);for(;;)lab_wait();}
            if(lab_request(&job,CMD_LOAD_FILE,300)) {
                pending=0;grant_2m();*ga_reg_comcmd1=loading?FILE_QUAY:FILE_SKY;
                __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=CMD_LOAD_FILE;
                lab_line(loading?"LOADING QUAY...":"LOADING SKY...",1,2);
            }
        }
        lab_wait();
        if(ready_art)continue;
        int event=lab_poll(&job,*ga_reg_comstat0,*ga_reg_comstat1);
        if(event==1)*ga_reg_comcmd0=0;
        if(event<0){lab_line("LOAD TIMEOUT / RESET TO RETRY",1,2);for(;;)lab_wait();}
        if(event==2) {
            if(job.result!=CDROM_RESULT_OK || (*ga_reg_memmode&5u)!=1u || !load_picture(loading,(volatile u8 const *)WORD_RAM,loading?ASSET_QUAY_BYTES:ASSET_SKY_BYTES)) {
                lab_line("DISC / ASSET VALIDATION FAILED",1,2);for(;;)lab_wait();
            }
            if(!loading){loading=1;pending=1;}else {ready_art=1;prepare_frame();lab_wait();lab_display_on();}
        }
    }
}

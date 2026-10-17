#include "lab.h"
#include "shared.h"
#include "model.h"
#include "asset.h"
#include "../build/asset_sizes.h"
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/io.def.h>
#include <main/mmd.h>
#include <main/vdp.h>
#include <sub/cdrom.def.h>
#include <system.h>
/* Resident startup facts are exercised by the common target harness. */
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_actor[],res_panel[];
static LabJob job;
static u16 scene=1,pending=1,requested,auto_mode,loads,failed;
static u16 due,shared_ok,seen;
#define WORLD_TILE 0x200
#define ACTOR_TILE 0x180
#define PANEL_TILE 0x190
static char const * const names[]={"SUNLIT TERRACE","LANTERN GROTTO"};
static u32 const sizes[]={ASSET_GARDEN_BYTES,ASSET_CAVERN_BYTES};
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
void lab_screen(char const *title) {lab_line(title,2,1);}
static void count(u16 value,u16 x,u16 y) {
    if(value>9999)value=9999;
    char digits[5];u16 place=1000;
    for(u16 i=0;i<4;i++) {digits[i]='0';while(value>=place){value-=place;digits[i]++;}place=(i==0?100:(i==1?10:1));}
    digits[4]=0;lab_line(digits,x,y);
}
static void palette(volatile u8 const *data,u16 line) {
    for(u16 i=0;i<16;i++)bios_palette[line*16+i]=asset_word(data+32+i*2);
}
static void panels(void) {
    for(u16 y=0;y<28;y++)if(y<4 || y>=24) {
        /* Text replaces Plane A cells; keep its opaque panel behind it on B. */
        for(u16 plane=0;plane<2;plane++) {
            vdp_ctrl_32=to_vdp_addr((plane?BIOS_VDP_DEFAULT_PLANEB:BIOS_VDP_DEFAULT_PLANEA)+y*128)|VRAM_W;
            for(u16 x=0;x<40;x++)vdp_data_16=(u16)(PANEL_TILE|0x2000);
        }
    }
}
static u16 shared(void) {
    AssetView actor,panel;
    if(!asset_open(res_actor,ASSET_ACTOR_BYTES,&actor) || actor.kind!=1 || actor.width!=2 || actor.height!=3 || actor.entries!=2 || actor.tiles!=12)return 0;
    if(!asset_open(res_panel,ASSET_PANEL_BYTES,&panel) || panel.kind || panel.tiles!=1)return 0;
    for(u16 i=0;i<16;i++)if(asset_word(res_actor+32+i*2)!=asset_word(res_panel+32+i*2))return 0;
    palette(res_actor,1);
    lab_upload_work(res_actor+64,to_vdp_addr(ACTOR_TILE*32)|VRAM_W,actor.tile_bytes/2);
    lab_upload_work(res_panel+64,to_vdp_addr(PANEL_TILE*32)|VRAM_W,panel.tile_bytes/2);
    panels();return 1;
}
static u16 draw_scene(volatile u8 const *data,u32 length) {
    AssetView view;
    if(!asset_open(data,length,&view) || view.kind || view.width!=40 || view.height!=20 || view.tiles>768)return 0;
    /* Rendering is hidden during bulk DMA. No claim of seamless streaming. */
    disable_interrupts();lab_display_off();
    lab_upload_word((void const *)(data+64),to_vdp_addr(WORLD_TILE*32)|VRAM_W,view.tile_bytes/2);
    for(u16 y=0;y<20;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB+(y+4)*128)|VRAM_W;
        for(u16 x=0;x<40;x++) {
            u16 cell=asset_word(data+view.map_offset+(y*40u+x)*2u);
            vdp_data_16=(u16)((cell&0x1800u)|((cell&0x7FFu)+WORLD_TILE)|0x4000);
        }
    }
    palette(data,2);bios_vdp_update_flags|=BIOS_MASK_COPY_PALETTE;
    /* Preserve the BIOS cache as well as changing the actual backdrop register. */
    bios_vdp_regs[7]=0x8720;vdp_ctrl_16=0x8720;
    enable_interrupts();lab_wait();lab_display_on();
    lab_line("ASSET LAB",2,1);lab_line("                ",22,1);lab_line(names[requested],22,1);
    lab_line("A NEXT   B AUTO   C RELOAD",2,25);
    lab_line("TILES 0000/768  LOADS 0000  PAL 3/4",2,26);count(view.tiles,8,26);count(loads,24,26);
    lab_line("PNG > 4BPP > DISC > WORD RAM > VDP",2,2);
    return 1;
}
static void actor(void) {
    u16 frame=(main_ticks>>5)&1u;
    u16 phase=(main_ticks>>1)&127u;
    u16 x=(u16)(24+(phase<64?phase:127-phase)*4);
    u16 y=(u16)(122+((main_ticks>>4)&3u));
    disable_interrupts();
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    vdp_data_16=y+128;vdp_data_16=0x0600;
    vdp_data_16=(u16)(0xA000|ACTOR_TILE|frame*6u);vdp_data_16=x+128;
    enable_interrupts();
}
void main(void) {
    *bios_vblank_user=vblank_user;enable_interrupts();lab_display_off();lab_clear();lab_font();
    for(u16 i=1;i<16;i++)bios_palette[i]=0x0EEE;
    disable_interrupts();shared_ok=shared();enable_interrupts();
    bios_vdp_update_flags|=BIOS_MASK_COPY_PALETTE;
    lab_wait();lab_display_on();
    if(!shared_ok){lab_line("SHARED ASSET FAILED",2,1);for(;;)lab_wait();}
#ifdef ASSET_AUTOTEST
    auto_mode=1;
#endif
    for(;;) {
        lab_wait();actor();
        if(failed)continue;
        u16 pressed=bios_joy1_hit;
        if(pressed&PAD_B)auto_mode^=1;
        if(job.phase==LAB_IDLE && !*ga_reg_comstat0 && (pending || (pressed&(PAD_A|PAD_C)) || (auto_mode && (u16)(main_ticks-due)<0x8000))) {
            requested=(pressed&PAD_C)?scene:(u16)(scene^1);pending=0;
            if((*ga_reg_memmode&5u)!=1u){lab_line("WORD RAM OWNER ERROR",2,2);failed=1;continue;}
            if(lab_request(&job,CMD_LOAD_FILE,300)) {
                grant_2m();*ga_reg_comcmd1=requested?FILE_CAVERN:FILE_GARDEN;
                __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=CMD_LOAD_FILE;
                lab_line("LOADING PREPARED ART...              ",2,2);
            }
        }
        int event=lab_poll(&job,*ga_reg_comstat0,*ga_reg_comstat1);
        if(event==1)*ga_reg_comcmd0=0;
        if(event<0){lab_line("LOAD TIMEOUT  RESET TO RETRY",2,2);failed=1;continue;}
        if(event==2) {
            if(job.result!=CDROM_RESULT_OK || (*ga_reg_memmode&5u)!=1u){lab_line("DISC/OWNERSHIP FAILED",2,2);failed=1;continue;}
            loads++;
            if(!draw_scene((volatile u8 const *)WORD_RAM,sizes[requested])){lab_line("ASSET VALIDATION FAILED",2,2);failed=1;continue;}
            scene=requested;seen|=(u16)(1u<<scene);due=(u16)(main_ticks+240);
#ifdef ASSET_AUTOTEST
            if(loads>=4){auto_mode=0;lab_line("                                    ",2,2);lab_line(seen==3?"AUTO PASS  BOTH SCENES RELOADED":"AUTO TOUR FAILED",2,2);}
#endif
        }
    }
}

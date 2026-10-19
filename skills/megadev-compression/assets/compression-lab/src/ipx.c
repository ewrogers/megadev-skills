#include "lab.h"
#include "shared.h"
#include "stream.h"
#include "compress.h"
#include "../build/compression_assets.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_font[],res_actor[];
extern void stream_install(void);
#define ACTOR_TILE 0x80
/* Two 640-tile slots end exactly at the BIOS SAT (B800); no pattern/table overlap. */
#define SLOT0 0xC0
#define SLOT1 0x340
_Static_assert((SLOT1+STREAM_TILES)*32u==BIOS_VDP_DEFAULT_SPRLIST,"Pattern slots must end before SAT");
_Static_assert(ACTOR_TILE+12<=SLOT0 && 0x20+96<=ACTOR_TILE,"Resident art must not overlap streamed slots");
#define HUD_WORDS 240
#define UP_WORDS 160
#define DIRTY_MAX 24
/* Entire interrupt payload lives in Main Work RAM, including copied Word RAM bytes. */
typedef struct {u16 hud[HUD_WORDS],palette[64],sat[8],data[UP_WORDS],address,words,row,dirty[DIRTY_MAX][2],dirty_count,palette_mask;} Frame;
static Frame frames[2];
static u16 displayed[HUD_WORDS],displayed_palette[64];
static u16 maps[2][STREAM_MAP],palettes[64],stage,active=2,target,loaded_slot;
static u16 player_x=48,player_y=120,hz,paused,transition,map_row,fade=8,actor_tick;
static u16 errors,previous_pass,page,selection,codec,preloaded,apply;
#ifdef STREAM_AUTOTEST
static u16 tour_step,tour_ok;
#endif
static volatile u16 published,input_hits,upload_ack,video_fault;
static u16 tile_base(u16 slot){return slot?SLOT1:SLOT0;}
static void reg(u16 index,u16 value){u16 w=(u16)(0x8000u|(index<<8)|value);bios_vdp_regs[index]=w;vdp_ctrl_16=w;}
void lab_line(char const *s,u16 x,u16 y) {
    char line[41];u16 n=0;if(x>=40 || y>=28)return;
    while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
    lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128+x*2)|VRAM_W);
}
void lab_hex(u16 v,u16 x,u16 y){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;lab_line(s,x,y);}
void lab_screen(char const *s){lab_line(s,1,0);}
void vblank_user(void) {
    main_ticks++;*ga_reg_comcmd3=main_ticks;input_hits|=bios_joy1_hit;
    u16 index=published;if(!index)return;
    Frame const *f=&frames[index-1];
    if(f->words>UP_WORDS || f->dirty_count>DIRTY_MAX){video_fault=1;published=0;return;}
    /* Palette first, only changed lines, and only at the early blank window.
       A late callback leaves the displayed snapshot unchanged for retry. */
    u16 vertical=(u16)(*(volatile u16 *)0xC00008u>>8);
    if((vdp_ctrl_16&VDPSTAT_MASK_VBLANK_IN_PROGRESS) && vertical>=224 && vertical<232) {
        for(u16 line=0;line<4;line++)if(f->palette_mask&(1u<<line)) {
            vdp_ctrl_32=to_vdp_addr(line*32u)|CRAM_W;
            for(u16 i=0;i<16;i++) {
                u16 cell=line*16u+i;vdp_data_16=f->palette[cell];
                bios_palette[cell]=f->palette[cell];displayed_palette[cell]=f->palette[cell];
            }
        }
    }
    for(u16 i=0;i<f->dirty_count;i++) {
        u16 cell=f->dirty[i][0];
        if(cell>=HUD_WORDS){video_fault=1;published=0;return;}
        u16 row=cell/40u;row=row<2?row:(u16)(row+22);
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+row*128u+(cell%40u)*2u)|VRAM_W;
        vdp_data_16=f->dirty[i][1];displayed[cell]=f->dirty[i][1];
    }
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    for(u16 i=0;i<8;i++)vdp_data_16=f->sat[i];
    if(f->words) {
        if(f->row) {
            for(u16 r=0;r<4;r++) {
                vdp_ctrl_32=to_vdp_addr(f->address+r*128u)|VRAM_W;
                for(u16 x=0;x<40;x++)vdp_data_16=f->data[r*40u+x];
            }
        } else {
            vdp_ctrl_32=to_vdp_addr(f->address)|VRAM_W;
            for(u16 i=0;i<f->words;i++)vdp_data_16=f->data[i];
            upload_ack=f->words*2u;
        }
    }
    published=0;
}
static void text(Frame *f,u16 x,u16 y,char const *s){while(*s && x<40 && y<6){f->hud[y*40+x]=(u16)(0x8000|(u8)*s);x++;s++;}}
static void number(Frame *f,u16 x,u16 y,u16 v){char s[6];s[5]=0;for(u16 i=5;i;i--){s[i-1]=(char)('0'+v%10u);v/=10u;}for(u16 i=0;i<4 && s[i]=='0';i++)s[i]=' ';text(f,x,y,s);}
/* Explicit 16x16 product: Megadev links modules without implicit libgcc. */
static u16 mul16(u16 a,u16 b){u32 r=a;__asm__("mulu.w %1,%0":"+d"(r):"d"(b):"cc");return (u16)r;}
static u16 shade(u16 c,u16 amount){u16 result=0;for(u16 s=0;s<12;s+=4){u16 n=(c>>s)&14u;result|=(u16)(((mul16(n,amount))>>3)&14u)<<s;}return result;}
void stream_prepare(void) {
    Frame *f=&frames[stage];f->words=0;f->row=0;
    for(u16 i=0;i<HUD_WORDS;i++)f->hud[i]=0x8020;
    static char const * const pages[]={"COMPARE","LOAD","PRELOAD"};
    static char const * const samples[]={"GARDEN","CAVERN","RANDOM NOISE"};
    u16 asset=(u16)(selection*2u+codec),bytes=cp_sizes[asset];
    text(f,1,0,"COMPRESSION LAB");text(f,29,0,pages[page]);
    text(f,1,1,"UP/DN SAMPLE  B FORMAT  A RUN  C STOP");
    text(f,1,2,samples[selection]);text(f,15,2,codec?"KOS":"RAW");
    text(f,20,2,"FILE");number(f,25,2,bytes);text(f,31,2,"BYTES");
    text(f,1,3,"DISC");number(f,5,3,(u16)((bytes+2047u)>>11));text(f,11,3,"SECT");
    text(f,17,3,"RAM");number(f,21,3,(u16)(((bytes+2047u)&~2047u)+cp_outputs[asset]));text(f,27,3,"B USED");
    text(f,1,4,"LAST READ");number(f,10,4,stream_read_frames);text(f,15,4,"F DEC");
    if(stream_decode_ticks==0xffffu)text(f,20,4,"RANGE");else number(f,20,4,stream_decode_ticks);
    text(f,26,4,"T");text(f,29,4,hz==50?"PAL":"NTSC");
    static char const * const phases[]={"READY","GRANT","CD READ","RETIRE","CHECK","UPLOAD","DRAIN","FAULT"};
    text(f,1,5,stream_result>=SR_MISSING?"LOAD ERROR":stream_phase?phases[stream_phase]:preloaded?"A SHOW PRELOAD":paused?"PAUSED":"CHECK OK");
    text(f,17,5,"MAIN");number(f,21,5,main_ticks);text(f,27,5,"T=30.72US");
#ifdef STREAM_AUTOTEST
    if(tour_ok)text(f,1,5,"AUTO PASS     ");
#endif
    f->dirty_count=0;
    for(u16 i=0;i<HUD_WORDS && f->dirty_count<DIRTY_MAX;i++)if(displayed[i]!=f->hud[i]) {
        f->dirty[f->dirty_count][0]=i;f->dirty[f->dirty_count][1]=f->hud[i];f->dirty_count++;
    }
    for(u16 i=0;i<64;i++)f->palette[i]=i>=32?shade(palettes[i],fade):palettes[i];
    f->palette_mask=0;
    for(u16 i=0;i<64;i++)if(f->palette[i]!=displayed_palette[i])f->palette_mask|=(u16)(1u<<(i>>4));
    /* Resident actor art never changes with CD ownership. */
    f->sat[0]=(u16)(128+player_y);f->sat[1]=0x0600; /* 2x3 tile hardware sprite */
    f->sat[2]=(u16)(0xA000|(ACTOR_TILE+((actor_tick>>3)&1u)*6u)|0);f->sat[3]=(u16)(128+player_x);
    for(u16 i=4;i<8;i++)f->sat[i]=0;
    if(transition==2) {
        f->row=1;f->address=(u16)(BIOS_VDP_DEFAULT_PLANEB+(4+map_row)*128u);f->words=160;
        for(u16 i=0;i<160;i++) {
            u16 cell=maps[target][map_row*40u+i];
            f->data[i]=(u16)((cell&0x1800u)|((cell&0x7FFu)+tile_base(target))|((target+2u)<<13));
        }
        map_row+=4;if(map_row==20){active=target;transition=3;reg(7,(u16)((active+2u)*16u));}
    } else if(stream_phase==ST_UPLOAD) {
        if((*ga_reg_memmode&7u)!=1u){errors++;stream_cancel();}
        else {
            u16 remaining=(u16)(stream_cache_tiles*32u-stream_uploaded);
            u16 count=STREAM_CHUNK;if(remaining<count)count=remaining;
            f->words=count/2;f->address=(u16)(tile_base(loaded_slot)*32u+stream_uploaded);
            volatile unsigned char *p=(volatile unsigned char *)(WORD_RAM+CP_OUTPUT)+64+stream_uploaded;
            for(u16 i=0;i<f->words;i++)f->data[i]=stream_word(p+i*2u);
        }
    }
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static int request(u16 which) {
    if(stream_start(which)){loaded_slot=active==0?1:0;preloaded=0;return 1;}return 0;
}
static void initialize(void) {
    lab_display_off();lab_clear();
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    lab_upload_work(res_actor+64,to_vdp_addr(ACTOR_TILE*32)|VRAM_W,192);
    for(u16 i=0;i<64;i++)palettes[i]=i==1?0x0EEE:i==2?0x0220:0;
    for(u16 i=0;i<16;i++)palettes[16+i]=stream_word(res_actor+32+i*2u);
    bios_vdp_update_flags=0;reg(11,0);reg(17,0);reg(18,0);reg(7,2);
    vdp_ctrl_32=CRAM_W;for(u16 i=0;i<64;i++){bios_palette[i]=palettes[i];vdp_data_16=palettes[i];displayed_palette[i]=palettes[i];}
    /* Plane A is transparent in the playfield; only HUD rows use opaque font cells. */
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++)vdp_data_16=(y<2 || y>=24)?0x8020:0;
    }
    for(u16 i=0;i<HUD_WORDS;i++)displayed[i]=0x8020;
    lab_display_on();
}
static int area_is_black(void) {
    if(active>1)return 1;
    for(u16 i=0;i<16;i++)if(displayed_palette[(active+2u)*16u+i])return 0;
    return 1;
}
void main(void) {
    disable_interrupts();stream_install();
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    initialize();enable_interrupts();request(0);
    u16 previous=main_ticks;
    for(;;) {
        lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
        if(upload_ack){u16 bytes=upload_ack;upload_ack=0;stream_upload_done(bytes);}
        stream_main_tick(delta>=(u16)(hz*30u)?STREAM_TIMEOUT:(u16)(mul16(delta,hz==50?6:5)));
        if(stream_pass!=previous_pass) {
            previous_pass=stream_pass;
            for(u16 i=0;i<STREAM_MAP;i++)maps[loaded_slot][i]=stream_cache_map[i];
            for(u16 i=0;i<16;i++)palettes[(loaded_slot+2u)*16u+i]=stream_cache_palette[i];
            target=loaded_slot;preloaded=1;
            if(active==2){fade=0;transition=2;map_row=0;preloaded=0;}
            else if(page!=2 || apply){transition=1;preloaded=0;apply=0;}
        }
#ifdef STREAM_AUTOTEST
        hit=0;
        if(active<2 && !transition && !stream_phase) {
            if(tour_step<6){selection=tour_step/2u;codec=tour_step&1u;request(tour_step++);}
            else if(tour_step==6){page=2;request(1);tour_step++;}
            else if(tour_step==7 && preloaded){target=loaded_slot;transition=1;preloaded=0;tour_step++;}
            else if(tour_step==8){page=1;selection=1;codec=1;apply=1;transition=4;tour_step++;}
            else if(tour_step==9)tour_ok=stream_pass==9 && !stream_errors && !video_fault;
        }
#endif
        if(hit&PAD_START)paused^=1;
        if(hit&PAD_RIGHT){page++;if(page==3)page=0;}
        if(hit&PAD_LEFT)page=page?(u16)(page-1):2;
        if(hit&(PAD_UP|PAD_DOWN)){selection++;if(selection==3)selection=0;}
        if(hit&PAD_B)codec^=1;
        if(hit&PAD_C){stream_cancel();preloaded=0;apply=0;}
        if((hit&PAD_A) && !transition && !stream_phase) {
            if(preloaded){target=loaded_slot;transition=1;preloaded=0;}
            else if(page==1){apply=1;transition=4;}
            else request((u16)(selection*2u+codec));
        }
        if(!paused){actor_tick++;player_x=(u16)(48+((main_ticks>>3)&31u));}
        if(transition==1 || transition==4) {
            if(fade)fade--;
            else if(area_is_black()) {
                if(transition==4){transition=0;request((u16)(selection*2u+codec));}
                else {transition=2;map_row=0;}
            }
        }
        if(!transition && !stream_phase && !fade && stream_result>=SR_CANCEL)transition=3;
        if(transition==3){if(fade<8)fade++;else transition=0;}
        stream_prepare();
    }
}

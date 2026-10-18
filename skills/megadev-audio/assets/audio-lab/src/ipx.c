#include "lab.h"
#include "shared.h"
#include "stream.h"
#include "audio.h"
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
static u16 maps[2][STREAM_MAP],palettes[64],cached[2],stage,active=2,target,loaded_slot;
static u16 player_x=48,player_y=120,hz,transition,map_row,fade=8,actor_tick;
static u16 errors,previous_pass,page,selection,pan_selection=1,music_started;
#ifdef AUDIO_AUTOTEST
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
    main_ticks++;input_hits|=bios_joy1_hit;
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
static void number(Frame *f,u16 x,u16 y,u16 v){char s[5];for(u16 i=0;i<4;i++)s[i]="0123456789ABCDEF"[(v>>(12-i*4))&15];s[4]=0;text(f,x,y,s);}
/* Explicit 16x16 product: Megadev links modules without implicit libgcc. */
static u16 mul16(u16 a,u16 b){u32 r=a;__asm__("mulu.w %1,%0":"+d"(r):"d"(b):"cc");return (u16)r;}
static u16 shade(u16 c,u16 amount){u16 result=0;for(u16 s=0;s<12;s+=4){u16 n=(c>>s)&14u;result|=(u16)(((mul16(n,amount))>>3)&14u)<<s;}return result;}
void stream_prepare(void) {
    Frame *f=&frames[stage];f->words=0;f->row=0;
    for(u16 i=0;i<HUD_WORDS;i++)f->hud[i]=0x8020;
    static char const * const pages[]={"CD MUSIC","PCM DESK","DRIVE TEST"};
    static char const * const samples[]={"BELL","PLUCK","DRUM"};
    static char const * const pans[]={"LEFT","CENTER","RIGHT"};
    static char const * const cds[]={"STOPPED","PLAYING","PAUSED","DATA LOAD","COMMAND","FAULT"};
    u16 status=*ga_reg_comstat4,cd=status>>8;
    text(f,1,0,"AUDIO LAB");text(f,26,0,pages[page]);
    text(f,1,1,"LEFT/RIGHT PAGE    START SILENCE");
    if(page==0) {
        text(f,1,2,selection?"03 AFTER RAIN":"02 HARBOR LANTERNS");
        text(f,1,3,"A PLAY   B PAUSE/RESUME   C STOP");
    } else if(page==1) {
        text(f,1,2,samples[selection]);text(f,12,2,"PAN");text(f,17,2,pans[pan_selection]);
        text(f,1,3,"A PLAY   B PAN   C PCM BED");
    } else {
        text(f,1,2,stream_phase?"READ / CHECK / UPLOAD IN PROGRESS":"A LOADS THE OTHER AREA");
        text(f,1,3,"A LOAD   B CD PLAY   C PCM BED");
    }
    text(f,1,4,"PCM");text(f,5,4,status&1?"BED":"---");text(f,9,4,status&2?"FX1":"---");text(f,13,4,status&4?"FX2":"---");
    text(f,20,4,"CD");text(f,24,4,cd<=5?cds[cd]:"STARTING");
    text(f,1,5,"MAIN");number(f,6,5,main_ticks);text(f,13,5,"LOADS");number(f,19,5,stream_pass);
    text(f,25,5,"ERR");number(f,29,5,(u16)(stream_errors+errors+video_fault+audio_link_fault+(cd==5)));
    text(f,34,5,hz==50?"PAL":"NTSC");
#ifdef AUDIO_AUTOTEST
    if(tour_ok){for(u16 i=200;i<HUD_WORDS;i++)f->hud[i]=0x8020;text(f,1,5,"AUTO PASS - AUDIO CONTINUES");}
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
    f->sat[2]=(u16)(0xA000|(ACTOR_TILE+((actor_tick>>3)&1u)*6u));f->sat[3]=(u16)(128+player_x);
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
            volatile unsigned char *p=(volatile unsigned char *)WORD_RAM+64+stream_uploaded;
            for(u16 i=0;i<f->words;i++)f->data[i]=stream_word(p+i*2u);
        }
    }
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static int request(u16 which) {
    if(stream_start(which)){loaded_slot=which;if(which<2)cached[which]=0;return 1;}return 0;
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
        audio_main_tick(delta>=(u16)(hz*30u)?9000:(u16)(mul16(delta,hz==50?6:5)));
        if(upload_ack){u16 bytes=upload_ack;upload_ack=0;stream_upload_done(bytes);}
        stream_main_tick(delta>=(u16)(hz*30u)?STREAM_TIMEOUT:(u16)(mul16(delta,hz==50?6:5)));
        if(stream_pass!=previous_pass) {
            previous_pass=stream_pass;cached[loaded_slot]=1;
            for(u16 i=0;i<STREAM_MAP;i++)maps[loaded_slot][i]=stream_cache_map[i];
            for(u16 i=0;i<16;i++)palettes[(loaded_slot+2u)*16u+i]=stream_cache_palette[i];
            target=loaded_slot;
            if(active==2){fade=0;transition=2;map_row=0;}else transition=1;
        }
        if(active<2 && !music_started && !audio_pending){audio_request(AU_PLAY,2);music_started=1;}
#ifdef AUDIO_AUTOTEST
        static u16 due,seen_data;
        hit=0;page=2;
        if((*ga_reg_comstat4>>8)==3 && (*ga_reg_comstat4&1))seen_data=1;
        if(active<2 && !transition && !stream_phase && !audio_pending && (u16)(main_ticks-due)<0x8000) {
            static u16 const ops[]={AU_BED,AU_EFFECT,AU_PAN,AU_EFFECT,AU_PAN,AU_EFFECT,AU_PAUSE,AU_PAUSE,0,AU_PLAY,AU_STOP};
            static u16 const args[]={0,0,0,1,2,2,0,0,0,3,0};
            if(tour_step<11) {
                u16 op=ops[tour_step];
                if(op?audio_request(op,args[tour_step]):request(active^1u)){tour_step++;due=(u16)(main_ticks+hz);}
            } else tour_ok=stream_pass>=2 && seen_data && !stream_errors && !errors && !video_fault && (*ga_reg_comstat4>>8)!=5;
        }
#endif
        if(hit&PAD_RIGHT){page++;if(page==3)page=0;selection=0;}
        if(hit&PAD_LEFT){page=page? (u16)(page-1):2;selection=0;}
        if(hit&(PAD_UP|PAD_DOWN)){selection++;if(selection>=(page==1?3:2))selection=0;}
        if(hit&PAD_START)audio_request(AU_PANIC,0);
        else if(page==0) {
            if(hit&PAD_A)audio_request(AU_PLAY,(u16)(selection+2));
            else if(hit&PAD_B)audio_request(AU_PAUSE,0);
            else if(hit&PAD_C)audio_request(AU_STOP,0);
        } else if(page==1) {
            if(hit&PAD_A)audio_request(AU_EFFECT,selection);
            else if(hit&PAD_B){pan_selection++;if(pan_selection==3)pan_selection=0;audio_request(AU_PAN,pan_selection);}
            else if(hit&PAD_C)audio_request(AU_BED,0);
        } else {
            if((hit&PAD_A) && active<2 && !transition)request(active^1u);
            else if(hit&PAD_B)audio_request(AU_PLAY,2);
            else if(hit&PAD_C)audio_request(AU_BED,0);
        }
        actor_tick++;player_x=(u16)(48+((main_ticks>>3)&31u));
        if(transition==1){if(fade)fade--;else if(area_is_black()){transition=2;map_row=0;}}
        if(transition==3){if(fade<8)fade++;else transition=0;}
        stream_prepare();
    }
}

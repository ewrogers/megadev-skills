#include "lab.h"
#include "shared.h"
#include "stream.h"
#include "audio.h"
#include "replay.h"
Replay replay;
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
typedef struct {u16 hud[HUD_WORDS],palette[64],sat[32],data[UP_WORDS],address,words,row,dirty[DIRTY_MAX][2],dirty_count,palette_mask;} Frame;
static Frame frames[2];
static u16 displayed[HUD_WORDS],displayed_palette[64];
static u16 maps[2][STREAM_MAP],palettes[64],cached[2],stage,active=2,target,loaded_slot;
static u16 player_x=48,player_y=120,hz,transition,map_row,fade=8,actor_tick;
static u16 errors,previous_pass,page,music_started;
static u16 bug,slow_upload,clock_fault,suite,suite_due,suite_pass,suite_fail,suite_baseline;
#ifdef AUDIO_AUTOTEST
static u16 tour_ok;
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
    for(u16 i=0;i<32;i++)vdp_data_16=f->sat[i];
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
    static char const * const modes[]={"LIVE","RECORDING","REPLAYING","MATCH","DIVERGED","FULL","INVALID"};
    u16 status=*ga_reg_comstat4,cd=status>>8;
    text(f,1,0,"TEST LAB");text(f,15,0,page?"LIVE CD + AUDIO":"REPLAY COURSE");
    text(f,1,1,page?"A RUN SUITE  B SLOW  C CANCEL":"A REC/STOP  B REPLAY  C BUG");
    if(!page) {
        text(f,1,2,modes[replay.mode<=RP_INVALID?replay.mode:RP_INVALID]);
        text(f,15,2,"TICKS");number(f,21,2,replay.length);text(f,27,2,"BUG");text(f,32,2,bug?"ON":"OFF");
        text(f,1,3,"FIRST BAD");if(replay.first_bad==65535)text(f,11,3,"----");else number(f,11,3,replay.first_bad);text(f,19,3,"GOALS");number(f,25,3,replay.state.goals);
        text(f,1,4,"BEACONS");number(f,9,4,replay.state.beacons);text(f,15,4,"GATE");text(f,20,4,replay.state.gate==1?"OPEN":"WAIT");
        text(f,1,5,"D-PAD MOVE / START CD + AUDIO");
    } else {
        text(f,1,2,suite==0?"READY":suite==7?"SUITE PASS":suite==8?"SUITE FAILED":"CHECKING LOAD / AUDIO HANDOFF");
        text(f,1,3,"PASS");number(f,6,3,suite_pass);text(f,13,3,"FAIL");number(f,19,3,suite_fail);
        text(f,26,3,slow_upload?"64B/BLANK":"256B/BLANK");
        text(f,1,4,"CD");text(f,4,4,cd==3?"DATA":cd==1?"MUSIC":cd==5?"FAULT":"WAIT");
        text(f,12,4,"LOADS");number(f,18,4,stream_pass);text(f,24,4,"RESULT");number(f,31,4,stream_result);
        text(f,1,5,"START COURSE / REAL DRIVE TIMING");
    }
    text(f,34,5,hz==50?"PAL":"NTSC");
    if(clock_fault || video_fault || stream_errors || audio_link_fault){text(f,1,5,"ERROR / CHECK CLOCK OR SERVICE");}
#ifdef AUDIO_AUTOTEST
    if(tour_ok){for(u16 i=200;i<HUD_WORDS;i++)f->hud[i]=0x8020;text(f,1,5,"AUTO PASS / REPLAY + LIVE I/O");}
#endif
    f->dirty_count=0;
    for(u16 i=0;i<HUD_WORDS && f->dirty_count<DIRTY_MAX;i++)if(displayed[i]!=f->hud[i]) {
        f->dirty[f->dirty_count][0]=i;f->dirty[f->dirty_count][1]=f->hud[i];f->dirty_count++;
    }
    for(u16 i=0;i<64;i++)f->palette[i]=i>=32?shade(palettes[i],fade):palettes[i];
    f->palette_mask=0;
    for(u16 i=0;i<64;i++)if(f->palette[i]!=displayed_palette[i])f->palette_mask|=(u16)(1u<<(i>>4));
    /* Resident actor/markers stay independent of Word RAM ownership. */
    for(u16 i=0;i<32;i++)f->sat[i]=0;
    f->sat[0]=(u16)(128+player_y);f->sat[1]=0x0601;
    f->sat[2]=(u16)(0xA000|(ACTOR_TILE+((actor_tick>>3)&1u)*6u));f->sat[3]=(u16)(128+player_x);
    u16 n=1;
    if(!page) {
        for(u16 i=0;i<3;i++)if(!(replay.state.beacons&(1u<<i))) {
            f->sat[n*4]=128+118;f->sat[n*4+1]=n+1;f->sat[n*4+2]=0x808C;f->sat[n*4+3]=(u16)(128+80+i*72);n++;
        }
        f->sat[n*4]=(u16)(128+replay.state.hazard_y+4);f->sat[n*4+1]=n+1;f->sat[n*4+2]=0x808D;f->sat[n*4+3]=128+164;n++;
        f->sat[n*4]=128+116;f->sat[n*4+1]=0;f->sat[n*4+2]=replay.state.gate==1?0x808E:0x808F;f->sat[n*4+3]=128+284;
    } else f->sat[1]=0x0600;
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
            u16 count=slow_upload?64:STREAM_CHUNK;if(remaining<count)count=remaining;
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
    /* Four original 8x8 QA markers, native pixels in the shared font palette. */
    vdp_ctrl_32=to_vdp_addr(0x8C*32u)|VRAM_W;
    for(u16 shape=0;shape<4;shape++)for(u16 y=0;y<8;y++)for(u16 x=0;x<8;x+=4) {
        u16 word=0;
        for(u16 k=0;k<4;k++) {
            u16 px=x+k,on=shape==0?((px==3 || px==4 || y==3 || y==4) && px>0 && px<7 && y>0 && y<7):
                shape==1?(px==y || px+y==7):shape==2?(px==y/2+2 || px+y/2==5):(px==1 || px==6 || y==1 || y==6);
            word=(u16)((word<<4)|(on?(shape==0?3:shape==1?4:shape==2?5:2):0));
        }
        vdp_data_16=word;
    }
    for(u16 i=0;i<64;i++)palettes[i]=i==1?0x0EEE:i==2?0x0222:i==3?0x06CE:i==4?0x022E:i==5?0x06E6:0;
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
void suite_tick(void) {
    if(!suite || suite>=7)return;
    if((u16)(main_ticks-suite_due)>hz*40u || stream_errors || audio_link_fault || (*ga_reg_comstat4>>8)==5){suite_fail++;suite=8;return;}
    if(transition || stream_phase || audio_pending)return;
    u16 cd=*ga_reg_comstat4>>8;
    if(suite==1){if(audio_request(AU_PLAY,2))suite=2;}
    else if(suite==2 && cd==1){suite_baseline=stream_pass;if(request(active^1u))suite=3;}
    else if(suite==3 && stream_pass!=suite_baseline && cd==1){if(audio_request(AU_PLAY,3)){suite_pass++;suite=4;}}
    else if(suite==4 && cd==1){if(stream_start(2))suite=5;}
    else if(suite==5 && stream_result==SR_MISSING && cd==1){if(request(active^1u)){suite_pass++;suite=6;}}
    else if(suite==6 && stream_ready && stream_result==SR_OK && cd==1){suite_pass++;suite=7;}
}
#ifdef AUDIO_AUTOTEST
void testing_tour(u16 delta) {
    static u16 step;
    page=1;
    u16 elapsed=delta>5?31:(u16)(delta*(hz==50?6u:5u));
    if(step==0 && active<2 && !transition){rp_record(&replay);step=1;}
    if(step==1) {
        if(!rp_advance(&replay,elapsed,RP_RIGHT,0))clock_fault=1;
        if(replay.state.tick>=240){rp_stop(&replay);rp_play(&replay);step=2;}
    } else if(step==2) {
        if(!rp_advance(&replay,elapsed,0,0))clock_fault=1;
        if(replay.mode==RP_MATCH){rp_play(&replay);step=3;}
    } else if(step==3) {
        rp_tick(&replay,0,0,1);
        if(replay.mode==RP_DIVERGED){rp_play(&replay);step=4;}
    } else if(step==4) {
        if(!rp_advance(&replay,elapsed,0,0))clock_fault=1;
        if(replay.mode==RP_MATCH){suite=1;suite_due=main_ticks;step=5;}
    } else if(step==5)tour_ok=suite==7 && !video_fault && !clock_fault;
}

#endif
void main(void) {
    disable_interrupts();stream_install();
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    rp_init(&replay,0xACE1);initialize();enable_interrupts();request(0);
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
        if(hit&PAD_START)page^=1;
        if(!page) {
            if(hit&PAD_A){if(replay.mode==RP_RECORD)rp_stop(&replay);else rp_record(&replay);}
            if(hit&PAD_B)rp_play(&replay);
            if(hit&PAD_C)bug^=1;
            u16 held=bios_joy1_hold&15u;
            u16 elapsed=delta>5?31:(u16)(delta*(hz==50?6u:5u));
            if(!rp_advance(&replay,elapsed,held,bug))clock_fault=1;
            player_x=replay.state.x;player_y=replay.state.y;actor_tick=replay.state.tick;
        } else {
            if((hit&PAD_A) && (!suite || suite>=7) && !stream_phase && !audio_pending && !transition){suite=1;suite_due=main_ticks;}
            if(hit&PAD_B)slow_upload^=1;
            if(hit&PAD_C){stream_cancel();suite=0;}
            actor_tick++;player_x=56;player_y=112;
        }
        suite_tick();
#ifdef AUDIO_AUTOTEST
        testing_tour(delta);
#endif
        if(transition==1){if(fade)fade--;else if(area_is_black()){transition=2;map_row=0;}}
        if(transition==3){if(fade<8)fade++;else transition=0;}
        stream_prepare();
    }
}

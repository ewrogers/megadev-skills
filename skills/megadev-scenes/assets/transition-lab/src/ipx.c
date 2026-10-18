#include "lab.h"
#include "shared.h"
#include "stream.h"
#include "audio.h"
#include "scene.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_font[],res_actor[],res_beacon[];
extern void stream_install(void);
#define ACTOR_TILE 0x80
/* Two 640-tile slots end exactly at the BIOS SAT (B800); no pattern/table overlap. */
#define SLOT0 0xC0
#define SLOT1 0x340
_Static_assert((SLOT1+STREAM_TILES)*32u==BIOS_VDP_DEFAULT_SPRLIST,"Pattern slots must end before SAT");
_Static_assert(ACTOR_TILE+16<=SLOT0 && 0x20+96<=ACTOR_TILE,"Resident art must not overlap streamed slots");
#define HUD_WORDS 240
#define UP_WORDS 160
#define DIRTY_MAX 24
/* Entire interrupt payload lives in Main Work RAM, including copied Word RAM bytes. */
typedef struct {u16 hud[HUD_WORDS],palette[64],sat[8],data[UP_WORDS],address,words,row,dirty[DIRTY_MAX][2],dirty_count,palette_mask;} Frame;
static Frame frames[2];
static u16 displayed[HUD_WORDS],displayed_palette[64];
static u16 maps[2][STREAM_MAP],palettes[64],cached[2],stage,active=2,target,loaded_slot;
static u16 player_x=48,player_y=120,hz,transition,map_row,fade=8,actor_tick;
static u16 errors,previous_pass,menu,speed,booted,load_started,preload_generation,inject_missing;
static u16 music_wanted=2,music_sent;
Scene flow;
#ifdef SCENE_AUTOTEST
static u16 tour_step,tour_ok,due;
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
    static char const * const names[]={"TITLE","LANTERN QUAY","MOON GROTTO","OPTIONS","CREDITS","JOURNEY COMPLETE"};
    text(f,1,0,"TRANSITION LAB");text(f,22,0,names[flow.current]);
    if(!booted)text(f,1,1,"PREPARING RESIDENT VIEW");
    else if(flow.phase!=SC_RUN)text(f,1,1,flow.stalled?"WAITING - RESOURCES STILL OWNED":"TRAVELLING - PLEASE WAIT");
    else if(scene_world(flow.current))text(f,1,1,"A BEACON   B PRELOAD   C TITLE");
    else text(f,1,1,"A SELECT   B BACK");
    if(flow.current==SC_TITLE) {
        text(f,1,2,"UP/DOWN CHOOSE YOUR DESTINATION");
        text(f,2,3,menu==0?"> START":"  START");text(f,13,3,menu==1?"> OPTIONS":"  OPTIONS");text(f,26,3,menu==2?"> CREDITS":"  CREDITS");
        text(f,1,4,"C TESTS A MISSING AREA AND RETURNS");
    } else if(flow.current==SC_OPTIONS) {
        text(f,1,2,"FLIGHT SPEED");text(f,16,2,speed?"BRISK":"GENTLE");
        text(f,1,3,"LEFT/RIGHT CHANGE   B TITLE");
        text(f,1,4,"THIS SETTING SURVIVES SCENE CHANGES");
    } else if(flow.current==SC_CREDITS) {
        text(f,1,2,"TRANSITION LAB - MEGADEV SKILLS");
        text(f,1,3,"ORIGINAL ART AND MUSIC FROM OUR LABS");
        text(f,1,4,"TECHNIQUES TO ADAPT TO YOUR GAME");
    } else if(flow.current==SC_RESULTS) {
        text(f,1,2,"BOTH LANTERNS FOUND - WELL TRAVELLED");
        text(f,1,3,"A TITLE   B TRAVEL AGAIN");
    } else {
        text(f,1,2,flow.room_value?"LANTERN FOUND - A TO CONTINUE":"FLY RIGHT TO THE WHITE BEACON");
        text(f,1,3,flow.paused?"PAUSED - START RESUME OR C TITLE":"D-PAD FLY   START PAUSE");
        text(f,1,4,stream_phase?"PRELOAD WORK IS STILL OWNED":cached[flow.current==SC_QUAY?1:0]?"NEXT AREA READY":"B PREPARES THE NEXT AREA EARLY");
    }
    text(f,1,5,"CHANGES");number(f,9,5,flow.transitions);text(f,15,5,"LOAD ERR");number(f,24,5,flow.failures);
    text(f,30,5,hz==50?"PAL":"NTSC");
#ifdef SCENE_AUTOTEST
    if(tour_ok){for(u16 i=200;i<HUD_WORDS;i++)f->hud[i]=0x8020;text(f,1,5,"AUTO PASS - CLEANUP AND RECOVERY");}
#endif
    f->dirty_count=0;
    for(u16 i=0;i<HUD_WORDS && f->dirty_count<DIRTY_MAX;i++)if(displayed[i]!=f->hud[i]) {
        f->dirty[f->dirty_count][0]=i;f->dirty[f->dirty_count][1]=f->hud[i];f->dirty_count++;
    }
    for(u16 i=0;i<64;i++)f->palette[i]=i>=16?shade(palettes[i],fade):palettes[i];
    f->palette_mask=0;
    for(u16 i=0;i<64;i++)if(f->palette[i]!=displayed_palette[i])f->palette_mask|=(u16)(1u<<(i>>4));
    /* Resident actor art never changes with CD ownership. */
    f->sat[0]=(u16)(128+player_y);f->sat[1]=0x0601;
    f->sat[2]=(u16)(0xA000|(ACTOR_TILE+((actor_tick>>3)&1u)*6u));f->sat[3]=(u16)(128+player_x);
    u16 bob=(actor_tick>>3)&7u;bob=bob<4?bob:7-bob;
    f->sat[4]=(u16)(128+120-bob);f->sat[5]=0x0500;f->sat[6]=(u16)(0x8000|(ACTOR_TILE+12));f->sat[7]=128+264;
    if(!booted || !scene_world(flow.current) || flow.phase!=SC_RUN || flow.paused)f->sat[4]=0;
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
    lab_upload_work(res_beacon,to_vdp_addr((ACTOR_TILE+12)*32)|VRAM_W,64);
    for(u16 i=0;i<64;i++)palettes[i]=i==1?0x0EEE:i==2?0x0220:0;
    palettes[3]=0x02CE; /* Gold center; white outline is palette entry 1. */
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
static void goto_scene(u16 id) {
    if(scene_request(&flow,id) && stream_phase)stream_cancel();
}
/* Only the main loop calls this coordinator; VBlank only consumes copied data. */
void scene_service(u16 elapsed) {
    u16 events=0;
    if(area_is_black())events|=SC_BLACK;
    if(!stream_phase && !*ga_reg_comcmd0 && !*ga_reg_comstat0 && (*ga_reg_memmode&7u)==1u)events|=SC_RETIRED;
    if(preload_generation && (events&SC_RETIRED)){scene_release(&flow,preload_generation);preload_generation=0;}
    if(flow.phase==SC_DRAIN && stream_phase)stream_cancel();
    u16 asset=flow.target==SC_CAVERN?1:0;
    if(flow.phase==SC_LOAD) {
        if(!load_started){if(request(inject_missing?2:asset))load_started=1;}
        else if(events&SC_RETIRED) {
            if(cached[asset] && !inject_missing)events|=SC_READY;
            else events|=SC_FAILED;
        }
    }
    if(flow.phase==SC_COMMIT && !flow.leases && !published) {
        if(transition==3){transition=0;events|=SC_PUBLISHED;}
        else if(!scene_world(flow.target) || (active==asset && !transition))events|=SC_PUBLISHED;
        else if(!transition){target=asset;map_row=0;transition=2;}
    }
    u16 old_phase=flow.phase;
    scene_tick(&flow,elapsed,events,inject_missing?0:cached[asset]);
    if(old_phase==SC_COMMIT && flow.phase==SC_IN){
        load_started=0;inject_missing=0;player_x=48;player_y=120;
        music_wanted=flow.current==SC_CAVERN?3:2;
    }
    fade=flow.fade;
}
void main(void) {
    disable_interrupts();stream_install();
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    scene_init(&flow);fade=0;flow.fade=0;
    initialize();enable_interrupts();request(0);
    u16 previous=main_ticks;
    for(;;) {
        lab_wait();u16 delta=(u16)(main_ticks-previous);previous=main_ticks;
        u16 elapsed=delta>=(u16)(hz*30u)?9000:(u16)mul16(delta,hz==50?6:5);
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
        audio_main_tick(elapsed);
        if(upload_ack){u16 bytes=upload_ack;upload_ack=0;stream_upload_done(bytes);}
        stream_main_tick(elapsed);
        if(stream_pass!=previous_pass) {
            previous_pass=stream_pass;cached[loaded_slot]=1;
            for(u16 i=0;i<STREAM_MAP;i++)maps[loaded_slot][i]=stream_cache_map[i];
            for(u16 i=0;i<16;i++)palettes[(loaded_slot+2u)*16u+i]=stream_cache_palette[i];
            if(!booted){target=loaded_slot;transition=2;map_row=0;}
        }
        if(!booted && transition==3){transition=0;booted=1;flow.phase=SC_IN;}
        if(booted)scene_service(elapsed);
        if(music_sent!=music_wanted && !audio_pending && audio_request(AU_PLAY,music_wanted))music_sent=music_wanted;
#ifdef SCENE_AUTOTEST
        hit=0;
        if(booted && flow.phase==SC_RUN && (u16)(main_ticks-due)<0x8000) {
            static u16 const route[]={SC_OPTIONS,SC_TITLE,SC_CREDITS,SC_TITLE,SC_QUAY,SC_TITLE,SC_CAVERN,SC_RESULTS,SC_TITLE};
            if(tour_step<9){
                if(tour_step==5 && scene_hold(&flow,flow.generation)) {
                    preload_generation=flow.generation;
                    if(!request(1)){scene_release(&flow,preload_generation);preload_generation=0;}
                }
                goto_scene(route[tour_step++]);due=(u16)(main_ticks+hz);
            }
            else if(tour_step==9){inject_missing=1;cached[0]=0;goto_scene(SC_QUAY);tour_step++;due=(u16)(main_ticks+hz);}
            else if(flow.failures==1 && flow.current==SC_TITLE && flow.transitions==9 && stream_dropped>=2 && !errors && !video_fault && !audio_link_fault && (*ga_reg_comstat4>>8)!=5)tour_ok=1;
        }
#endif
        if(booted && flow.phase==SC_RUN) {
            if(scene_world(flow.current)) {
                if(hit&PAD_START)scene_pause(&flow);
                if(hit&PAD_C)goto_scene(SC_TITLE);
                else if(!flow.paused) {
                    u16 step=speed?3:2;
                    if((bios_joy1_hold&PAD_LEFT) && player_x>16)player_x-=step;
                    if((bios_joy1_hold&PAD_RIGHT) && player_x<280)player_x+=step;
                    if((bios_joy1_hold&PAD_UP) && player_y>40)player_y-=step;
                    if((bios_joy1_hold&PAD_DOWN) && player_y<160)player_y+=step;
                    actor_tick++;
                    if(player_x>=248 && player_y>=96 && player_y<=144)flow.room_value=1;
                    if((hit&PAD_A) && flow.room_value){audio_request(AU_EFFECT,0);goto_scene(flow.current==SC_QUAY?SC_CAVERN:SC_RESULTS);}
                    else if((hit&PAD_B) && !stream_phase && !cached[flow.current==SC_QUAY?1:0]) {
                        if(scene_hold(&flow,flow.generation)) {
                            preload_generation=flow.generation;
                            if(!request(flow.current==SC_QUAY?1:0)){scene_release(&flow,preload_generation);preload_generation=0;}
                        }
                    }
                }
            } else {
                actor_tick++;
                if(flow.current==SC_TITLE) {
                    if(hit&PAD_DOWN){menu++;if(menu==3)menu=0;}
                    if(hit&PAD_UP)menu=menu?menu-1:2;
                    if(hit&(PAD_A|PAD_START))goto_scene(menu==0?SC_QUAY:menu==1?SC_OPTIONS:SC_CREDITS);
                    if(hit&PAD_C){inject_missing=1;cached[0]=0;goto_scene(SC_QUAY);}
                } else if(flow.current==SC_OPTIONS) {
                    if(hit&(PAD_LEFT|PAD_RIGHT))speed^=1;
                    if(hit&(PAD_B|PAD_C))goto_scene(SC_TITLE);
                } else if(flow.current==SC_RESULTS && (hit&PAD_B))goto_scene(SC_QUAY);
                else if(hit&(PAD_A|PAD_B|PAD_C|PAD_START))goto_scene(SC_TITLE);
            }
        }
        stream_prepare();
    }
}

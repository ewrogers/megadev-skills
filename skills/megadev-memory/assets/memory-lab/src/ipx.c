#include "lab.h"
#include "shared.h"
#include "memory_link.h"
#include <main/io.def.h>
#include <main/bios.h>
#include <main/gate_arr.h>
#include <main/memmap.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic=0x12345678;
volatile u32 main_zero[8];
volatile u16 main_ticks,room_runs;
extern u8 const res_font[];
#define CELLS 1120
#define DIRTY_CAP 96
/* Only the callback writes VDP ports during normal display. */
typedef struct {u16 offset,word;} Cell;
typedef struct {Cell cells[DIRTY_CAP];u16 count;u16 sat[8][4],sprites;} Frame;
_Static_assert(sizeof(Frame)==452,"Publisher fixture layout changed");
static u16 desired[CELLS],displayed[CELLS],scan,stage,hz,page;
static Frame frames[2];
static volatile u16 published,submitted,video_fault,input_hits;
extern void memory_install(void);
extern u8 _ROM_LENGTH[],_RAM_DATA_LENGTH[],_BSS_LENGTH[];
/* Volatile makes the diagnostic writes observable instead of an unused mock. */
static volatile u8 scene_pool[4096];
static MArena arena;
static MHandle scene_handles[3];
static MLease scene_lease;
static u16 life_step_index,life_failures,life_blocks,stale_rejected;
#ifdef MEMORY_AUTOTEST
static u16 tour_step,tour_due=60,tour_finished,tour_ok,checks;
#endif
void lab_line(char const *s,u16 x,u16 y) {
    char line[41];u16 n=0;
    if(x>=40 || y>=28)return;
    while(s[n] && n<40-x){line[n]=s[n];n++;}line[n]=(char)0xFF;
    lab_print(line,to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128+x*2)|VRAM_W);
}
void lab_hex(u16 value,u16 x,u16 y) {
    static char const h[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=h[(value>>(12-i*4))&15];
    s[4]=0;lab_line(s,x,y);
}
void lab_screen(char const *title){lab_line(title,1,0);}
void vblank_user(void) {
    main_ticks++;input_hits|=bios_joy1_hit;
    u16 index=published;if(!index)return;
    Frame const *f=&frames[index-1];
    if(f->count>DIRTY_CAP || f->sprites>8){video_fault=1;published=0;return;}
    for(u16 i=0;i<f->count;i++)if(f->cells[i].offset>=CELLS){video_fault=1;published=0;return;}
    for(u16 i=0;i<f->count;i++) {
        u16 cell=f->cells[i].offset;
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(cell/40u)*128u+(cell%40u)*2u)|VRAM_W;
        vdp_data_16=f->cells[i].word;displayed[cell]=f->cells[i].word;
    }
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    u16 sprites=f->sprites?f->sprites:1;
    for(u16 i=0;i<sprites;i++)for(u16 w=0;w<4;w++)vdp_data_16=f->sat[i][w];
    submitted++;published=0;
}
static void text(u16 x,u16 y,u16 color,char const *s) {
    while(*s && x<40 && y<28){desired[y*40+x]=(u16)(0x8000|(color<<13)|(u8)*s);x++;s++;}
}
static void hex(u16 x,u16 y,u16 color,u16 value) {
    static char const h[]="0123456789ABCDEF";char s[5];
    for(u16 i=0;i<4;i++)s[i]=h[(value>>(12-i*4))&15];
    s[4]=0;text(x,y,color,s);
}

void life_step(void) {
    switch(life_step_index) {
    case 0:
        if(!ma_take(&arena,512,2,&scene_handles[0]) ||
           !ma_take(&arena,2048,16,&scene_handles[1]) ||
           !ma_take(&arena,1024,32,&scene_handles[2])){video_fault=1;return;}
        for(u16 i=0;i<scene_handles[2].size;i++)scene_pool[scene_handles[2].offset+i]=(u8)i;
        if(!ma_pin(&arena,&scene_handles[2],&scene_lease)){video_fault=1;return;}
        break;
    case 1: {
        MHandle extra={0};
        if(ma_take(&arena,1024,2,&extra))video_fault=1;else life_failures++;
        break;
    }
    case 2:
        if(ma_reset(&arena)) {
            stale_rejected=!ma_valid(&arena,&scene_handles[2]);life_step_index=5;return;
        }
        life_blocks++;
        break;
    case 3:
        if(scene_lease.active && !ma_release(&arena,&scene_lease))video_fault=1;
        break;
    case 4:
        if(!ma_reset(&arena))video_fault=1;
        stale_rejected=!ma_valid(&arena,&scene_handles[2]);break;
    default:
        return;
    }
    life_step_index++;
}
static void bar(u16 x,u16 y,u16 used,u16 total) {
    for(u16 i=0;i<32;i++)text(x+i,y,(u16)(i*total)<(u16)(used*32u)?1:0,
        (u16)(i*total)<(u16)(used*32u)?"+":"-");
}
static void draw(void) {
    for(u16 i=0;i<CELLS;i++)desired[i]=0x8020;
    text(1,0,0,"MEMORY LAB");text(24,0,1,page==0?"RAM BUDGETS":page==1?"LIFETIMES":"WORD RAM");
    for(u16 i=0;i<40;i++)text(i,1,0,"-");
    text(1,2,0,"LEFT / RIGHT OR START CHANGES PAGE");
    if(page==0) {
        text(1,4,0,"MAIN WORK RAM  64 KIB");
        bar(3,5,241,256);text(1,7,0,"LINKED CODE");hex(16,7,1,(u16)((u32)_ROM_LENGTH+(u32)_RAM_DATA_LENGTH));
        text(1,8,0,"LINKED STATE");hex(16,8,1,(u16)((u32)_BSS_LENGTH+(u32)_RAM_DATA_LENGTH));
        text(1,9,2,"SCENE POOL IS INCLUDED IN STATE");
        text(1,11,0,"SUB PROGRAM RAM  512 KIB");
        bar(3,12,128,512);text(1,14,0,"UPPER 384 KIB NOT ASSIGNED HERE");
        text(1,16,0,"WORD RAM  256 KIB");
        text(1,17,1,(*ga_reg_memmode&4u)?"1M MODE: TWO 128 KIB BANKS":"2M MODE: ONE 256 KIB REGION");
        text(1,19,0,"CODE / STATE / STACK / BIOS RESERVED");
        text(1,20,0,"SEE LAYOUT REPORT FOR EXACT BYTES");
        text(1,22,2,"PCM 64 KIB  Z80 8 KIB  VRAM 64 KIB");
        text(1,23,0,"SEPARATE POOLS - NOT GENERAL HEAP");
    } else if(page==1) {
        text(1,4,0,"SCENE ARENA  4096 BYTES");
        bar(3,5,(u16)(arena.used>>7),32);
        text(1,7,0,"USED");hex(8,7,1,(u16)arena.used);text(20,7,0,"PEAK");hex(28,7,1,(u16)arena.peak);
        text(1,8,0,"ACTIVE LEASES");hex(20,8,2,arena.pins);
        text(1,10,0,life_step_index==0?"A LOAD A SCENE":
            life_step_index==1?"A TRY AN ALLOCATION THAT WONT FIT":
            life_step_index==2?"A TRY TO UNLOAD WHILE PINNED":
            life_step_index==3?"A FINISH THE DEFERRED USER":
            life_step_index==4?"A UNLOAD AND INVALIDATE HANDLES":"LIFETIME CHECK COMPLETE");
        text(1,12,0,"REFUSED ALLOCS");hex(22,12,2,life_failures);
        text(1,13,0,"BLOCKED UNLOADS");hex(22,13,2,life_blocks);
        text(1,14,0,"OLD HANDLE");text(22,14,stale_rejected?1:0,stale_rejected?"REJECTED":"NOT TESTED");
        text(1,17,0,"A NEXT STEP    B RESET    C RELEASE");
        text(1,19,0,"A QUEUED USER HOLDS A LEASE");
        text(1,20,0,"UNLOAD WAITS UNTIL THAT USER FINISHES");
        text(1,22,2,"OFFSETS ARE NOT CROSS-CPU POINTERS");
    } else {
        u16 mode=*ga_reg_memmode;
        text(1,4,0,(mode&4u)?"1M MODE: MAIN AND SUB OWN A BANK":"2M MODE: ONE CPU OWNS THE REGION");
        if(mode&4u) {
            text(2,6,(mode&1u)?2:1,(mode&1u)?"BANK 0: SUB":"BANK 0: MAIN");
            text(21,6,(mode&1u)?1:2,(mode&1u)?"BANK 1: MAIN":"BANK 1: SUB");
        } else text(2,6,(mode&1u)?1:2,(mode&1u)?"WHOLE REGION: MAIN":"WHOLE REGION: SUB");
        text(1,8,0,"RESIDENT SNAPSHOT OF MAIN DATA");
        for(u16 y=0;y<4;y++)for(u16 x=0;x<16;x++)text((u16)(4+x*2),(u16)(10+y),(memory_preview[y*16+x]>>3)&3,"+");
        text(1,15,0,"BACK BANK");text(16,15,back_ready?1:0,back_ready?"READY TO SWAP":"NOT READY");
        text(1,16,0,"COMMAND");text(16,16,memory_phase==4?3:1,memory_phase==0?"IDLE":memory_phase==4?"FAULT - RESET DISC":"WAITING");
        text(1,18,0,"A PREPARE    B CHANGE 1M / 2M");
        text(1,19,0,"C SWAP BANKS WHEN READY");
        text(1,21,2,"MAIN READS ITS BANK WHILE SUB FILLS");
        text(1,22,0,"SWAPS WAIT FOR THE MAIN READER");
        text(1,23,0,"MODE CHANGES REBUILD THE CONTENTS");
    }
    text(1,25,0,"OK");hex(5,25,1,memory_pass);text(15,25,0,"ERRORS");hex(24,25,memory_errors?3:1,memory_errors);
    text(1,26,0,"MAIN");hex(6,26,1,main_ticks);text(15,26,0,"SUB");hex(19,26,1,*ga_reg_comstat3);
    text(34,26,0,hz==50?"PAL":"NTSC");
#ifdef MEMORY_AUTOTEST
    text(1,27,tour_ok?1:2,tour_finished?(tour_ok?"AUTO PASS - DISPLAY CONTINUES":"AUTO FAILED"):"AUTOMATIC MEMORY TOUR");
#else
    text(1,27,0,"NUMBERS ARE HEX / BUDGETS IN KIB");
#endif
}
static void publish(void) {
    Frame *f=&frames[stage];f->count=0;f->sprites=0;
    u16 checked=0;
    while(checked<CELLS && f->count<DIRTY_CAP) {
        u16 cell=scan;scan++;if(scan==CELLS)scan=0;checked++;
        if(desired[cell]!=displayed[cell]){f->cells[f->count].offset=cell;f->cells[f->count].word=desired[cell];f->count++;}
    }
    for(u16 i=0;i<4;i++)f->sat[0][i]=0;
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize_video(void) {
    lab_display_off();lab_clear();
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    u16 const colors[4]={0x0EEE,0x06E6,0x06CE,0x068E};
    for(u16 p=0;p<4;p++)for(u16 i=0;i<16;i++)bios_palette[p*16+i]=i==1?(s16)colors[p]:(i==2?0x0220:0);
    bios_vdp_update_flags=0;vdp_ctrl_32=CRAM_W;
    for(u16 i=0;i<64;i++)vdp_data_16=(u16)bios_palette[i];
}
void main(void) {
    disable_interrupts();memory_install();ma_init(&arena,sizeof scene_pool,1);
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    initialize_video();draw();
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){u16 cell=y*40+x;displayed[cell]=desired[cell];vdp_data_16=desired[cell];}
    }
    lab_display_on();enable_interrupts();
    u16 previous_tick=main_ticks;
    for(;;) {
        lab_wait();
        u16 delta=(u16)(main_ticks-previous_tick);previous_tick=main_ticks;
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
#ifdef MEMORY_AUTOTEST
        hit=0;page=2;
        if(life_step_index<5)life_step();
        if(!tour_finished && tour_step<9 && !memory_phase && (u16)(main_ticks-tour_due)<0x8000) {
            static u16 const operations[]={MM_FILL2,MM_ENTER1,MM_SWAP1,MM_FILL1,MM_SWAP1,MM_FILL1,MM_SWAP1,MM_EXIT2,MM_FILL2};
            if(memory_start(operations[tour_step])){tour_step++;tour_due=(u16)(main_ticks+30);}
        }
        if(tour_step==9 && !memory_phase) {
            tour_finished=1;tour_ok=memory_pass==9 && !memory_errors && !video_fault && checks>0 && stale_rejected && life_blocks==1 && life_failures==1;
        }
#endif
        if(hit&(PAD_RIGHT|PAD_START)){page++;if(page==3)page=0;}
        if(hit&PAD_LEFT){if(!page)page=2;else page--;}
        if(page==1) {
            if(hit&PAD_A)life_step();
            if(hit&PAD_B){if(ma_reset(&arena))life_step_index=0;else life_blocks++;}
            if(hit&PAD_C)ma_release(&arena,&scene_lease);
        } else if(page==2) {
            if(hit&PAD_A)memory_start((*ga_reg_memmode&4u)?MM_FILL1:MM_FILL2);
            if(hit&PAD_B)memory_start((*ga_reg_memmode&4u)?MM_EXIT2:MM_ENTER1);
            if(hit&PAD_C)memory_start(MM_SWAP1);
        }
        memory_main_tick(delta>=(u16)(hz*5u)?1500:(u16)(delta*(hz==50?6u:5u)));
        if(front_valid && memory_front_check()) {
#ifdef MEMORY_AUTOTEST
            checks++;
#endif
        }
        draw();publish();
    }
}

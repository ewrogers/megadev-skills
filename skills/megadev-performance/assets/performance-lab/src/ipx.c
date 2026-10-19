#include "lab.h"
#include "shared.h"
#include "scale.h"
#include "perf.h"
PfWindow perf_window;
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
static u16 desired[CELLS],displayed[CELLS],stage,hz;
static u16 dirty_cells[CELLS],dirty_head,dirty_tail,dirty_count;
static unsigned char dirty_mark[CELLS];
static ScRequest request={1,0,2,0};

static u16 perf_case,desired_case,level=1,method=1,hud=1,timer_on=1,run_request=1;
static u16 phase,last_phase,last_count,last_repeats,check_ok=1,stamp_overhead,metric_fault;
static u16 job_tick,job_active,job_blanks,previous_completed,case_dirty=1,preparing;
static volatile u16 upload_kib,measure_uploads;
static u16 report_images,image_base;
static Pf32 work_checksum;
static volatile u16 dma_offset,front_image,image_count,views_seen;
extern u16 const scale_palette[];
static Frame frames[2];
static volatile u16 published,submitted,video_fault,input_hits;
extern void scaling_install(void);
#ifdef SCALING_AUTOTEST
static u16 tour_ok;
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
    /* At most 4096 image bytes + 512 map bytes + 384 HUD bytes in
       an early VBlank. A late callback defers the whole image upload. */
    u16 vline=*(volatile u16 *)0xC00008u>>8;
    if(scale_upload && (*(volatile u16 *)0xC00004u&8u) && vline>=224 && vline<=230) {
        u16 tile=(u16)(256u+(front_image^1u)*256u);
        lab_upload_work(scale_pixels+dma_offset,to_vdp_addr(tile*32u+dma_offset*2u)|VRAM_W,SC_UPLOAD_WORDS);
        if(measure_uploads)upload_kib+=SC_UPLOAD_WORDS/512u;
        dma_offset+=SC_UPLOAD_WORDS;
        if(dma_offset==SC_WORDS) {
            for(u16 y=0;y<16;y++) {
                vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(y+4u)*128u+12u*2u)|VRAM_W;
                for(u16 x=0;x<16;x++){u16 word=(u16)(0xA000u+tile+x*16u+y);vdp_data_16=word;displayed[(y+4u)*40u+12u+x]=word;}
            }
            front_image^=1u;image_count++;views_seen|=(u16)(1u<<scale_last_mode);dma_offset=0;
            __asm__ volatile("" ::: "memory");scale_upload=0;
        }
    }
    for(u16 i=0;i<f->count;i++) {
        u16 cell=f->cells[i].offset;
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+(cell/40u)*128u+(cell%40u)*2u)|VRAM_W;
        vdp_data_16=f->cells[i].word;displayed[cell]=f->cells[i].word;
    }
    vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST)|VRAM_W;
    for(u16 w=0;w<4;w++)vdp_data_16=f->sat[0][w];
    submitted++;published=0;
}
static void text(u16 x,u16 y,u16 color,char const *s) {
    if(color==1)color=2; /* Palette 1 belongs to art, never HUD glyphs. */
    u16 cell=(u16)(y*40u+x);
    while(*s && x<40 && y<28) {
        u16 value=(u16)(0x8000|(color<<13)|(u8)*s);
        if(desired[cell]!=value || displayed[cell]!=value) {
            desired[cell]=value;
            if(!dirty_mark[cell]) {
                if(dirty_count==CELLS){video_fault=1;return;}
                dirty_mark[cell]=1;dirty_cells[dirty_tail]=cell;
                dirty_tail++;if(dirty_tail==CELLS)dirty_tail=0;dirty_count++;
            }
        }
        cell++;x++;s++;
    }
}
static void decimal(u16 x,u16 y,u16 value) {
    char s[6];s[5]=0;for(u16 i=5;i;i--){s[i-1]=(char)('0'+value%10u);value/=10u;}for(u16 i=0;i<4 && s[i]=='0';i++)s[i]=' ';text(x,y,0,s);
}
static char const *case_name(u16 which) {
    static char const * const names[]={"MAIN CPU","VRAM UPLOAD","ASIC PIPELINE","CPU + ASIC + VRAM"};return names[which<4?which:0];
}
void scaling_draw(void) {
    static u16 last_hud_tick;
    if(!case_dirty){
        if(!perf_window.running || !hud)return;
        if((u16)(main_ticks-last_hud_tick)<hz/10u)return;
    }
    last_hud_tick=main_ticks;
    if(case_dirty){
        for(u16 y=0;y<28;y++) {
            if(y>=4 && y<20){text(0,y,0,"            ");text(28,y,0,"            ");}
            else text(0,y,0,"                                        ");
        }
        if(perf_case==PF_CPU){for(u16 y=4;y<20;y++)text(12,y,0,"                ");text(15,9,1,"MAIN WORK");text(14,12,0,"FIXED INPUTS");}
        case_dirty=0;
    }
    text(1,0,0,"PERFORMANCE LAB");text(1,1,2,case_name(perf_case));
    text(29,0,0,hz==50?"PAL / 50":"NTSC / 60");
    text(1,3,0,"METHOD");text(9,3,1,method?"INCREMENTAL":"REMAINDER  ");
    text(23,3,0,"CPU LOAD");decimal(32,3,level+1);
    text(1,5,0,"MAIN LOOPS");decimal(1,6,perf_window.loops);
    text(1,8,0,"LATE BLANKS");decimal(1,9,perf_window.late);
    text(1,11,0,"64W BLOCKS");decimal(1,12,perf_window.blocks);
    text(1,14,0,"UPLOAD KIB");decimal(1,15,upload_kib);
    text(29,5,0,"IMAGES");decimal(29,6,report_images);
    text(29,8,0,"WORK TICKS");if(perf_window.samples)decimal(29,9,perf_window.last);else text(29,9,0,"-----");
    text(29,11,0,"MIN / MAX");if(perf_window.samples){decimal(29,12,perf_window.minimum);decimal(29,13,perf_window.maximum);}else {text(29,12,0,"-----");text(29,13,0,"-----");}
    text(29,15,0,"READ PAIR");if(timer_on)decimal(29,16,stamp_overhead);else text(29,16,0,"-----");
    text(1,20,0,"WINDOW BLANKS");decimal(15,20,perf_window.elapsed);
    text(23,20,0,"JOB BLANKS");decimal(34,20,job_blanks);
    text(1,21,0,"STATUS");text(9,21,2,perf_window.running?"MEASURING 4S":run_request?"DRAIN / WAIT":"DONE        ");
    text(1,25,0,"CHECK");text(8,25,check_ok && !metric_fault && !scale_error && !video_fault?1:3,check_ok && !metric_fault && !scale_error && !video_fault?"OK  ":"FAIL");
    text(15,25,0,"REJECTED");decimal(25,25,perf_window.rejected);
    text(1,22,0,"A RUN  B METHOD  C HUD");text(30,22,1,hud?"HUD ON ":"HUD OFF");
    text(1,23,0,"L/R CASE   UP/DOWN CPU LOAD");
    text(1,24,0,"START TIMER");text(14,24,1,timer_on?"ON ":"OFF");
    text(1,26,0,"ELAPSED TICKS / NOT CPU UTILIZATION");
#ifdef SCALING_AUTOTEST
    text(1,27,1,tour_ok?"AUTO PASS / FOUR WORKLOADS":"AUTOMATIC FOUR CASE TOUR");
#endif
}
void scaling_publish(void) {
    Frame *f=&frames[stage];f->count=0;f->sprites=0;
    while(dirty_count && f->count<DIRTY_CAP) {
        u16 cell=dirty_cells[dirty_head];dirty_mark[cell]=0;
        dirty_head++;if(dirty_head==CELLS)dirty_head=0;dirty_count--;
        if(desired[cell]!=displayed[cell]){f->cells[f->count].offset=cell;f->cells[f->count].word=desired[cell];f->count++;}
    }
    f->sat[0][0]=(u16)(128+112+scale_sine((u16)(main_ticks*2u))/16);
    f->sat[0][1]=0x0500;f->sat[0][2]=0xC080;f->sat[0][3]=128+276;
    disable_interrupts();__asm__ volatile("" ::: "memory");published=stage+1;enable_interrupts();stage^=1;
}
static void initialize_video(void) {
    lab_display_off();lab_clear();
    lab_upload_work(res_font,to_vdp_addr(0x20*32)|VRAM_W,96*16);
    u16 const colors[4]={0x0EEE,0x06E6,0x06CE,0x068E};
    for(u16 p=0;p<4;p++)for(u16 i=0;i<16;i++)bios_palette[p*16+i]=i==1?(s16)colors[p]:(i==2?0x0220:0);
    bios_vdp_update_flags=0;vdp_ctrl_32=CRAM_W;
    for(u16 i=0;i<16;i++)bios_palette[16+i]=(s16)scale_palette[i];
    for(u16 i=0;i<64;i++)vdp_data_16=(u16)bios_palette[i];
    /* Original 16px foreground wisp, independent of the ASIC output. */
    vdp_ctrl_32=to_vdp_addr(128*32)|VRAM_W;
    for(u16 tx=0;tx<2;tx++)for(u16 ty=0;ty<2;ty++)for(u16 y=0;y<8;y++)for(u16 x=0;x<8;x+=4) {
        u16 word=0;
        for(u16 k=0;k<4;k++) {
            s16 dx=(s16)(tx*8+x+k)-7,dy=(s16)(ty*8+y)-7;
            if(dx<0)dx=-dx;
            if(dy<0)dy=-dy;
            word=(u16)((word<<4)|(dx+dy<7?(dx+dy<3?1:2):0));
        }
        vdp_data_16=word;
    }
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){desired[y*40+x]=displayed[y*40+x]=0x8020;vdp_data_16=0x8020;}
    }
}
static void queue_run(void){measure_uploads=0;perf_window.running=0;run_request=1;preparing=0;case_dirty=1;}
void scaling_input(u16 hit) {
    if(hit&PAD_A)queue_run();
    if(hit&PAD_LEFT){desired_case=(desired_case+3u)&3u;queue_run();}
    if(hit&PAD_RIGHT){desired_case=(desired_case+1u)&3u;queue_run();}
    if(hit&PAD_UP && level<3){level++;queue_run();}
    if(hit&PAD_DOWN && level){level--;queue_run();}
    if(hit&PAD_B){method^=1;queue_run();}
    if(hit&PAD_C){hud^=1;queue_run();}
    if(hit&PAD_START){timer_on^=1;queue_run();}
}
void perf_prepare_run(void) {
    if(!run_request || scale_phase || scale_upload)return;
    if(!preparing) {
        perf_case=desired_case;
        if(perf_case==PF_UPLOAD)for(u16 i=0;i<SC_WORDS;i++)scale_pixels[i]=(u16)(0x1234u^(i&15u)*0x1111u);
        perf_window=(PfWindow){0};upload_kib=0;report_images=0;work_checksum=0;last_count=0;
        phase=0;stamp_overhead=0;metric_fault=0;check_ok=1;request=(ScRequest){1,0,2,0};job_active=job_blanks=0;previous_completed=scale_completed;
        case_dirty=1;preparing=1;scaling_draw();return;
    }
    /* Prime the real ASIC/source snapshot outside the timed window. */
    if(preparing==1 && (perf_case==PF_ASIC || perf_case==PF_COMBINED)) {
        if(scale_start(&request))preparing=2;
        return;
    }
    /* Warm-up layout has reached VRAM before the measured window begins. */
    if(dirty_count || published)return;
    upload_kib=0;image_base=image_count;
    pf_begin(&perf_window,main_ticks,hz);measure_uploads=1;run_request=0;preparing=0;case_dirty=1;scaling_draw();
}
void perf_schedule(void) {
    if(!perf_window.running)return;
    if(perf_case==PF_UPLOAD){if(!scale_upload)scale_upload=1;}
    else if(perf_case==PF_ASIC || perf_case==PF_COMBINED) {
        if(scale_start(&request)){job_tick=main_ticks;job_active=1;request.angle=(request.angle+3u)&255u;request.motion++;}
    }
}
void perf_poll(u16 elapsed) {
    scale_main_tick(elapsed);
    if(scale_completed!=previous_completed){previous_completed=scale_completed;if(job_active){job_blanks=(u16)(main_ticks-job_tick);job_active=0;}}
}
void perf_work(void) {
    if(!perf_window.running || (perf_case!=PF_CPU && perf_case!=PF_COMBINED))return;
    last_phase=phase;last_count=pf_words(level);last_repeats=pf_repeats(level);
    work_checksum=pf_batch(phase,last_count,last_repeats,method);
    phase+=7;if(phase>=300)phase-=300;
}
void perf_finish(void) {
    if(last_count)check_ok=(work_checksum==pf_batch(last_phase,last_count,last_repeats,0));
    case_dirty=1;
}
void perf_sample(u16 delta) {
        u16 running=perf_window.running;
        u16 timed=timer_on && (perf_case==PF_CPU || perf_case==PF_COMBINED);
        u16 before_tick=main_ticks,start=0,end=0,after_tick=before_tick,coherent=1;
        if(running && timed){
            start=*ga_reg_stopwatch&PF_COUNTER_MASK;
            if(main_ticks!=before_tick)coherent=0;
        }
        perf_work();
        if(running && timed){
            after_tick=main_ticks;end=*ga_reg_stopwatch&PF_COUNTER_MASK;
            if(main_ticks!=after_tick)coherent=0;
        }
        if(running) {
            u16 ticks=0,valid=pf_elapsed(start,end,(u16)(after_tick-before_tick),coherent,&ticks);
            u16 blocks=(perf_case==PF_CPU || perf_case==PF_COMBINED)?(u16)((last_count>>6)*last_repeats):0;
            /* The first wake began during unmeasured preparation. */
            u16 observed_delta=perf_window.loops?delta:(u16)(main_ticks-perf_window.start);
            int result=pf_observe(&perf_window,main_ticks,observed_delta?observed_delta:1,blocks,ticks,valid,timed);
            report_images=(u16)(image_count-image_base);
            if(result<0)metric_fault=1;
            if(!perf_window.running){measure_uploads=0;perf_finish();}
            if(timer_on){u16 a=*ga_reg_stopwatch,b=*ga_reg_stopwatch;stamp_overhead=(b-a)&PF_COUNTER_MASK;}
        }
}
void main(void) {
    disable_interrupts();scaling_install();
#if VIDEO == PAL
    hz=50;
#else
    hz=60;
#endif
    initialize_video();scaling_draw();
    for(u16 y=0;y<28;y++) {
        vdp_ctrl_32=to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA+y*128)|VRAM_W;
        for(u16 x=0;x<40;x++){u16 cell=y*40+x;displayed[cell]=desired[cell];vdp_data_16=desired[cell];}
    }
    for(u16 i=0;i<CELLS;i++)dirty_mark[i]=0;
    dirty_head=dirty_tail=dirty_count=0;
    lab_display_on();enable_interrupts();
    u16 previous_tick=main_ticks;
    for(;;) {
        lab_wait();u16 delta=(u16)(main_ticks-previous_tick);previous_tick=main_ticks;
        disable_interrupts();u16 hit=input_hits;input_hits=0;enable_interrupts();
        perf_poll(delta>=hz?300:(u16)(delta*(hz==50?6u:5u)));
#ifdef SCALING_AUTOTEST
        static u16 tour_case,tour_done,tour_failed;
        hit=0;
        if(!perf_window.running && !run_request && !tour_done){
            if(!check_ok || scale_error || metric_fault || video_fault)tour_failed=1;
            if(tour_case<3){tour_case++;desired_case=tour_case;queue_run();}
            else {tour_done=1;tour_ok=!tour_failed;}
        }
#endif
        scaling_input(hit);perf_prepare_run();
        perf_sample(delta);
        perf_schedule();scaling_draw();scaling_publish();
        /* Preserve prompt handoff observation and snapshot/transfer overlap. */
        perf_poll(0);perf_schedule();
    }
}

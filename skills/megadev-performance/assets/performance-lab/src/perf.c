#include "perf.h"
_Static_assert(sizeof(Pf16)==2 && sizeof(Pf32)==4,"Benchmark integer widths changed");
static Pf16 value(Pf16 index){return (Pf16)((index^0x5A5Au)+(index<<3));}
int pf_valid(Pf16 phase,Pf16 count,Pf16 repeats,Pf16 method){return phase<300 && count && count<=512 && repeats && repeats<=4 && method<2;}
Pf32 pf_reference(Pf16 phase,Pf16 count){
    Pf32 sum=0x12345678u^phase;
    for(Pf16 i=0;i<count;i++){
        Pf16 numerator=(Pf16)(phase+i*13u);
        sum+=value((Pf16)(numerator%300u));
    }
    return sum;
}
Pf32 pf_incremental(Pf16 phase,Pf16 count){
    Pf32 sum=0x12345678u^phase;Pf16 cursor=phase;
    for(Pf16 i=0;i<count;i++){
        sum+=value(cursor);cursor+=13;if(cursor>=300)cursor-=300;
    }
    return sum;
}
Pf32 pf_batch(Pf16 phase,Pf16 count,Pf16 repeats,Pf16 method){
    if(!pf_valid(phase,count,repeats,method))return 0;
    Pf32 result=0;
    for(Pf16 i=0;i<repeats;i++){
        result^=method?pf_incremental(phase,count):pf_reference(phase,count);
        phase+=7;if(phase>=300)phase-=300;
        result=(result<<1)|(result>>31);
    }
    return result;
}
Pf16 pf_words(Pf16 level){return level<4?(Pf16)(64u<<level):0;}
Pf16 pf_repeats(Pf16 level){return level<4?(level<2?1:(Pf16)(1u<<(level-1))):0;}
/* A <=3 serviced-blank span fits within one 12-bit wrap at nominal PAL/NTSC.
   No reset detection is possible from two reads alone: qualify the clock owner. */
int pf_elapsed(Pf16 start,Pf16 end,Pf16 blanks,Pf16 coherent,Pf16 *ticks){
    if(!coherent || blanks>3)return 0;
    *ticks=(Pf16)((end-start)&PF_COUNTER_MASK);return 1;
}
int pf_begin(PfWindow *w,Pf16 tick,Pf16 hz){
    if(hz!=50 && hz!=60)return 0;
    *w=(PfWindow){tick,hz,1,0,0,0,0,0,0,0,65535,0};return 1;
}
int pf_observe(PfWindow *w,Pf16 tick,Pf16 delta,Pf16 blocks,Pf16 ticks,Pf16 valid,Pf16 timed){
    if(!w->running)return 0;
    Pf16 elapsed=(Pf16)(tick-w->start);
    if(elapsed>4096 || (w->loops && elapsed<=w->elapsed) || !delta || delta>32 || blocks>32){w->running=0;return -1;}
    w->elapsed=elapsed;w->loops++;w->late+=(Pf16)(delta-1);w->blocks+=blocks;
    if(timed){
        if(!valid)w->rejected++;
        else {w->samples++;w->last=ticks;if(ticks<w->minimum)w->minimum=ticks;if(ticks>w->maximum)w->maximum=ticks;}
    }
    if(elapsed>=w->hz*PF_SECONDS){w->running=0;return 2;}
    return 1;
}

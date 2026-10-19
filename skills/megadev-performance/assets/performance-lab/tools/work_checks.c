#include "perf.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    for(Pf16 phase=0;phase<300;phase++)for(Pf16 level=0;level<4;level++) {
        Pf16 n=pf_words(level),repeats=pf_repeats(level);
        assert(pf_batch(phase,n,repeats,0)==pf_batch(phase,n,repeats,1));
    }
    assert(!pf_batch(300,64,1,0) && !pf_batch(0,513,1,0) && !pf_batch(0,64,5,0));
    Pf16 ticks=999;assert(pf_elapsed(4090,7,1,1,&ticks) && ticks==13);
    assert(!pf_elapsed(4090,7,4,1,&ticks) && ticks==13);
    assert(!pf_elapsed(1,3,0,0,&ticks) && ticks==13);
    PfWindow w={0};assert(!pf_begin(&w,0,55));
    for(Pf16 hz=50;hz<=60;hz+=10){
        assert(pf_begin(&w,65500,hz));
        for(Pf16 i=1;i<=hz*4;i++)assert(pf_observe(&w,(Pf16)(65500+i),1,1,10,1,1)==(i==hz*4?2:1));
        assert(!w.running && w.blocks==hz*4 && w.elapsed==hz*4 && !w.late && w.minimum==10 && w.maximum==10);
        assert(!pf_observe(&w,1,1,1,0,0,1));
    }
    pf_begin(&w,0,60);assert(pf_observe(&w,2,2,2,0,0,1)==1 && w.rejected==1 && w.late==1 && !w.samples);
    assert(pf_observe(&w,2,1,0,0,1,1)==-1 && !w.running);
    pf_begin(&w,0,60);assert(pf_observe(&w,1,1,1,0,1,0)==1 && !w.samples && !w.rejected);
    assert(pf_observe(&w,40,39,1,0,1,1)==-1);
    printf("{\"passed\":true,\"equivalent_cases\":1200,\"reference_checksum\":\"%08lx\",\"timer_wrap_ticks\":13,\"timing_claim\":\"logic only; no hardware speed measurement\"}\n",(unsigned long)pf_batch(17,512,4,0));
    return 0;
}

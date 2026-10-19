#ifndef PERF_LAB_H
#define PERF_LAB_H
/* Application metrics and benchmark policy, not Megadev APIs. */
typedef unsigned short Pf16;
typedef unsigned int Pf32;
#define PF_COUNTER_MASK 4095u
#define PF_SECONDS 4u
enum { PF_CPU,PF_UPLOAD,PF_ASIC,PF_COMBINED };
typedef struct {
    Pf16 start,hz,running,elapsed,loops,late,blocks,samples,rejected,last,minimum,maximum;
} PfWindow;
int pf_valid(Pf16 phase,Pf16 count,Pf16 repeats,Pf16 method);
Pf32 pf_reference(Pf16 phase,Pf16 count);
Pf32 pf_incremental(Pf16 phase,Pf16 count);
Pf32 pf_batch(Pf16 phase,Pf16 count,Pf16 repeats,Pf16 method);
Pf16 pf_words(Pf16 level);
Pf16 pf_repeats(Pf16 level);
int pf_elapsed(Pf16 start,Pf16 end,Pf16 blanks,Pf16 coherent,Pf16 *ticks);
int pf_begin(PfWindow *w,Pf16 tick,Pf16 hz);
int pf_observe(PfWindow *w,Pf16 tick,Pf16 delta,Pf16 blocks,Pf16 ticks,Pf16 valid,Pf16 timed);
#endif

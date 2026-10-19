#include "scale.h"
static ScS32 mul16(ScS16 a,ScS16 b) {
#ifdef __m68k__
    /* Exact original-68000 signed 16x16 -> 32 operation. Prevent compiler
       reassociation into a 32x32 libgcc call; no ISA extension is required. */
    ScS32 product=a;
    __asm__("muls.w %1,%0" : "+d"(product) : "d"(b) : "cc");
    return product;
#else
    return (ScS32)a*b;
#endif
}
static ScS16 const sine[64]={0,25,50,74,98,121,142,162,181,198,213,226,237,245,251,255,256,255,251,245,237,226,213,198,181,162,142,121,98,74,50,25,0,-25,-50,-74,-98,-121,-142,-162,-181,-198,-213,-226,-237,-245,-251,-255,-256,-255,-251,-245,-237,-226,-213,-198,-181,-162,-142,-121,-98,-74,-50,-25};
ScS16 scale_sine(ScU16 angle) {
    ScU16 index=(angle&255u)>>2,fraction=angle&3u;
    ScS16 a=sine[index],b=sine[(index+1u)&63u];
    return (ScS16)(a+mul16((ScS16)(b-a),(ScS16)fraction)/4);
}
int scale_valid(ScRequest const *r){return r && r->mode<3 && r->angle<256 && r->zoom<5;}
int scale_trace(ScLine *out,ScU16 capacity,ScRequest const *r) {
    static ScU16 const depth[128]={256,240,227,215,204,195,186,178,170,163,157,151,146,141,136,132,128,124,120,117,113,110,107,105,102,99,97,95,93,91,89,87,85,83,81,80,78,77,75,74,73,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,56,55,54,53,53,52,51,51,50,49,49,48,48,47,47,46,46,45,45,44,44,43,43,42,42,41,41,40,40,40,39,39,39,38,38,37,37,37,36,36,36,35,35,35,35,34,34,34,33,33,33,33,32,32,32,32,31,31,31,31,30,30,30,30,29,29,29,29,29,28,28};
    static ScU16 const steps[5]={1024,1536,2048,3072,4096};
    if(!out || capacity<SC_HEIGHT || !scale_valid(r))return 0;
    ScS32 c=r->mode?scale_sine((ScU16)(r->angle+64u)):256;
    ScS32 s=r->mode?scale_sine(r->angle):0;
    for(ScU16 row=0;row<SC_HEIGHT;row++) {
        ScS32 step=steps[r->zoom],distance=(ScS32)row-64;
        if(r->mode==2) {
            /* Bounded integer reciprocal: an inverse ground-plane projection.
               The first line is below the horizon; no divide-by-zero. */
            distance=depth[row];
            step=mul16((ScS16)distance,(ScS16)(r->zoom+2u))*4;
        }
        ScS32 dx=mul16((ScS16)c,(ScS16)step)/256,dy=mul16((ScS16)s,(ScS16)step)/256;
        ScS32 u,v;
        if(r->mode==2) {
            ScS32 center=(ScS32)(r->motion&255u);
            u=128*2048 + mul16((ScS16)distance,(ScS16)(-s))*8 - 64*dx;
            v=center*2048 + mul16((ScS16)distance,(ScS16)c)*8 - 64*dy;
        } else {
            u=128*2048 -64*dx-mul16((ScS16)distance,(ScS16)dy);
            v=128*2048 -64*dy+mul16((ScS16)distance,(ScS16)dx);
        }
        /* Starts are signed/wrapping 13.3 words; steps are signed 5.11.
           Unsigned conversion is deliberate modulo 65536, not saturation. */
        out[row]=(ScLine){(ScU16)(u/256),(ScU16)(v/256),(ScU16)dx,(ScU16)dy};
    }
    return 1;
}

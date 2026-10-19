#include "math_core.h"
#ifdef __m68k__
#include <math.h> /* Megadev mul/mulu; division/fixed macros are not used. */
#endif
#include "sine_table.h"
static MmS32 product(MmS16 a,MmS16 b) {
#ifdef __m68k__
 return mul(a,b);
#else
 return (MmS32)a*b;
#endif
}
static MmU32 uproduct(MmU16 a,MmU16 b) {
#ifdef __m68k__
 return mulu(a,b);
#else
 return (MmU32)a*b;
#endif
}
/* Private: denominator nonzero and quotient <=32768 proved by callers. */
static MmU16 divide(MmU32 n,MmU16 d) {
#ifdef __m68k__
 __asm__("divu.w %1,%0":"+d"(n):"d"(d):"cc");return (MmU16)n;
#else
 return (MmU16)(n/d);
#endif
}
static MmU16 magnitude(MmS16 n){return n<0?(MmU16)(-(MmS32)n):(MmU16)n;}
static MmS32 trunc256(MmS32 n){return n<0?-(MmS32)((MmU32)(-n)>>8):(MmS32)((MmU32)n>>8);}
static MmU16 store(MmS32 n,MmS16 *out){if(!out || n<-32768 || n>32767)return 0;*out=(MmS16)n;return 1;}
MmU16 mm_from_int(MmS16 n,MmS16 *out){if(n<-128 || n>127 || !out)return 0;*out=(MmS16)(n*256);return 1;}
MmS16 mm_to_int(MmS16 n){return (MmS16)trunc256(n);}
MmU16 mm_add(MmS16 a,MmS16 b,MmS16 *out){return store((MmS32)a+b,out);}
MmU16 mm_sub(MmS16 a,MmS16 b,MmS16 *out){return store((MmS32)a-b,out);}
MmU16 mm_mul(MmS16 a,MmS16 b,MmS16 *out){return store(trunc256(product(a,b)),out);}
MmU16 mm_div(MmS16 a,MmS16 b,MmS16 *out) {
 if(!b || !out)return 0;
 MmU16 negative=(a<0)!=(b<0),d=magnitude(b),limit=negative?32768:32767;
 MmU32 n=(MmU32)magnitude(a)<<8;
 if(n>=uproduct((MmU16)(limit+1u),d))return 0;
 MmU16 q=divide(n,d);*out=(MmS16)(negative?-(MmS32)q:q);return 1;
}
MmU16 mm_isqrt(MmU32 n) {
 MmU32 root=0,bit=0x40000000u;
 /* Sixteen bounded restoring steps, no division or floating point. */
 for(MmU16 i=0;i<16;i++,bit>>=2) {
  if(n>=root+bit){n-=root+bit;root=(root>>1)+bit;}
  else root>>=1;
 }
 return (MmU16)root;
}
MmU32 mm_length2(MmVec const *v){return uproduct(magnitude(v->x),magnitude(v->x))+uproduct(magnitude(v->y),magnitude(v->y));}
MmU16 mm_normalize(MmVec const *v,MmVec *out) {
 if(!v || !out || (!v->x && !v->y))return 0;
 MmU16 x=magnitude(v->x),y=magnitude(v->y);
 /* Scale magnitudes up first: tiny vectors need more precision than floor(length). */
 while(x<16384 && y<16384){x=(MmU16)(x<<1);y=(MmU16)(y<<1);}
 MmU16 length=mm_isqrt(uproduct(x,x)+uproduct(y,y));
 MmU16 nx=divide((MmU32)x<<8,length),ny=divide((MmU32)y<<8,length);
 MmVec value={(MmS16)(v->x<0?-(MmS32)nx:nx),(MmS16)(v->y<0?-(MmS32)ny:ny)};*out=value;return 1;
}
MmU16 mm_dot(MmVec const *a,MmVec const *b,MmS32 *out) {
 if(!a || !b || !out)return 0;
 MmS32 x=product(a->x,b->x),y=product(a->y,b->y);
 if((y>0 && x>2147483647-y) || (y<0 && x<(-2147483647-1)-y))return 0;
 *out=x+y;return 1; /* Raw Q16.16; do not narrow to Q8.8 implicitly. */
}
MmS16 mm_sin(MmU16 angle) {
 MmU16 phase=angle&1023u,q=phase>>8,i=phase&255u;
 MmS16 n=(MmS16)mm_quarter[(q&1u)?256u-i:i];return q>=2?(MmS16)-n:n;
}
MmS16 mm_cos(MmU16 angle){return mm_sin((MmU16)(angle+256u));}
MmU16 mm_rotate(MmVec const *v,MmU16 angle,MmVec *out) {
 if(!v || !out)return 0;
 MmS16 c=mm_cos(angle),s=mm_sin(angle),x,y;
 if(!store(trunc256(product(v->x,c)-product(v->y,s)),&x) || !store(trunc256(product(v->x,s)+product(v->y,c)),&y))return 0;
 *out=(MmVec){x,y};return 1;
}
MmU16 mm_lerp(MmS16 a,MmS16 b,MmU16 t,MmS16 *out) {
 if(t>256 || !out)return 0;
 MmS32 delta=(MmS32)b-a;MmU16 absdelta=(MmU16)(delta<0?-delta:delta);
 MmS32 step=(MmS32)(uproduct(absdelta,t)>>8);
 *out=(MmS16)((MmS32)a+(delta<0?-step:step));return 1;
}
MmU16 mm_smoothstep(MmU16 t,MmU16 *out) {
 if(t>256 || !out)return 0;
 if(t==256){*out=256;return 1;}
 *out=(MmU16)(uproduct((MmU16)uproduct(t,t),(MmU16)(768u-2u*t))>>16);return 1;
}

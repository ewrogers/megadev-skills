#include "collision.h"

/* Every caller validates coordinates/motion first. Internal products fit signed
   32 bits; each factor magnitude fits 16 bits. Target uses base-68000 word
   multiply/divide, not absent libgcc long helpers or later CPU instructions. */
static CcS32 product(CcS32 a,CcS32 b)
{
#ifdef __m68k__
 CcU32 value=(CcU16)(a<0?-a:a);CcU16 factor=(CcU16)(b<0?-b:b);
 __asm__("mulu.w %1,%0":"+d"(value):"d"(factor):"cc");
 return ((a<0)!=(b<0))?-(CcS32)value:(CcS32)value;
#else
 return a*b;
#endif
}
static CcS32 quotient(CcS32 n,CcS32 d)
{
#ifdef __m68k__
 /* Positive denominator, quotient magnitude <=4096, so DIVU cannot overflow. */
 CcU32 value=(CcU32)(n<0?-n:n);CcU16 divisor=(CcU16)d;
 __asm__("divu.w %1,%0":"+d"(value):"d"(divisor):"cc");
 CcS32 q=(CcU16)value;return n<0?-q:q;
#else
 return n/d;
#endif
}
typedef struct {CcS32 n,d;} Ratio;
static CcHit result(CcU16 kind)
{
    CcHit h={kind,0,65535,0,0,0,1};return h;
}
static bool less(Ratio a,Ratio b){return product(a.n,b.d) < product(b.n,a.d);}
static bool equal(Ratio a,Ratio b){return product(a.n,b.d) == product(b.n,a.d);}
static bool position(CcS32 p){return p>=-CC_LIMIT && p<=CC_LIMIT;}
static bool valid(CcBox a,bool point)
{
    return position(a.x) && position(a.y) && a.w>=(point ? 0 : 1) &&
        a.h>=(point ? 0 : 1) && a.w<=CC_EXTENT && a.h<=CC_EXTENT;
}
static bool motion(CcBox a,CcVec d)
{
    return d.x>=-CC_MOTION && d.x<=CC_MOTION && d.y>=-CC_MOTION && d.y<=CC_MOTION &&
        position((CcS32)a.x+d.x) && position((CcS32)a.y+d.y);
}
bool cc_overlap(CcBox a,CcBox b)
{
    return valid(a,false) && valid(b,false) && a.x<(CcS32)b.x+b.w &&
        (CcS32)a.x+a.w>b.x && a.y<(CcS32)b.y+b.h && (CcS32)a.y+a.h>b.y;
}
static bool inside(CcBox a,CcBox b)
{
    return a.x<(CcS32)b.x+b.w && (CcS32)a.x+a.w>b.x &&
        a.y<(CcS32)b.y+b.h && (CcS32)a.y+a.h>b.y;
}
/* Test relative swept bounds before divisions. Includes end-face contact.
   Keep this target-tested boundary: inlining into cc_ray at -O2 failed the
   inside-point regression on the compiled original-68000 model. */
static __attribute__((noinline)) bool broad(CcBox a,CcS32 dx,CcS32 dy,CcBox b)
{
    CcS32 x0=a.x+(dx<0 ? dx : 0),y0=a.y+(dy<0 ? dy : 0);
    CcS32 x1=(CcS32)a.x+a.w+(dx>0 ? dx : 0),y1=(CcS32)a.y+a.h+(dy>0 ? dy : 0);
    return x0<=(CcS32)b.x+b.w && x1>=b.x && y0<=(CcS32)b.y+b.h && y1>=b.y;
}
static CcHit query(CcBox a,CcS32 dx,CcS32 dy,CcBox b)
{
    if(inside(a,b))return result(CC_OVERLAP);
    Ratio enter={-1,1},exit={2,1};CcS16 nx=0,ny=0;
    for(CcU16 axis=0;axis<2;axis++) {
        CcS32 p=axis ? a.y : a.x,size=axis ? a.h : a.w;
        CcS32 lo=axis ? b.y : b.x,hi=lo+(axis ? b.h : b.w),d=axis ? dy : dx;
        if(!d){if(p>=hi || p+size<=lo)return result(CC_MISS);continue;}
        Ratio en,ex;CcS16 normal;
        if(d>0){en=(Ratio){lo-p-size,d};ex=(Ratio){hi-p,d};normal=-1;}
        else{en=(Ratio){p-hi,-d};ex=(Ratio){p+size-lo,-d};normal=1;}
        if(less(enter,en)){enter=en;nx=axis ? 0 : normal;ny=axis ? normal : 0;}
        else if(equal(enter,en)){if(axis)ny=normal;else nx=normal;}
        if(less(ex,exit))exit=ex;
    }
    /* Grazing one corner for an instant does not enter the open interior. */
    if(enter.n<0 || enter.n>enter.d || !less(enter,exit))return result(CC_MISS);
    CcHit h=result(CC_HIT);h.num=enter.n;h.den=enter.d;
    h.time=(CcU16)(quotient(product(enter.n,CC_TIME),enter.d));h.nx=nx;h.ny=ny;return h;
}
CcHit cc_sweep(CcBox a,CcVec da,CcBox b,CcVec db)
{
    if(!valid(a,false) || !valid(b,false) || !motion(a,da) || !motion(b,db))return result(CC_INVALID);
    CcS32 dx=(CcS32)da.x-db.x,dy=(CcS32)da.y-db.y;
    if(!broad(a,dx,dy,b))return result(CC_MISS);
    return query(a,dx,dy,b);
}
CcHit cc_ray(CcVec start,CcVec delta,CcBox target)
{
    CcBox point={start.x,start.y,0,0};
    if(!valid(point,true) || !valid(target,false) || !motion(point,delta))return result(CC_INVALID);
    if(!broad(point,delta.x,delta.y,target))return result(CC_MISS);
    return query(point,delta.x,delta.y,target);
}
static CcHit cast(CcBox a,CcVec delta,const CcBox *walls,CcU16 count,CcStats *stats,bool point)
{
    CcStats local={0,0};if(!stats)stats=&local;*stats=local;
    if(!valid(a,point) || !motion(a,delta) || count>CC_MAX_BOXES || (count && !walls))return result(CC_INVALID);
    /* Validate the entire supplied list, even walls rejected by broad phase. */
    for(CcU16 i=0;i<count;i++)if(!valid(walls[i],false))return result(CC_INVALID);
    CcHit best=result(CC_MISS);
    for(CcU16 i=0;i<count;i++) {
        stats->visited++;
        if(!broad(a,delta.x,delta.y,walls[i]))continue;
        stats->narrow++;CcHit h=query(a,delta.x,delta.y,walls[i]);h.index=i;
        if(h.kind==CC_OVERLAP)return h;
        if(h.kind!=CC_HIT)continue;
        if(best.kind!=CC_HIT || less((Ratio){h.num,h.den},(Ratio){best.num,best.den}))best=h;
        else if(equal((Ratio){h.num,h.den},(Ratio){best.num,best.den})) {
            /* Simultaneous walls: combine axes; lowest list index wins metadata. */
            if(h.nx)best.nx=h.nx;
            if(h.ny)best.ny=h.ny;
        }
    }
    return best;
}
CcHit cc_cast(CcBox a,CcVec delta,const CcBox *walls,CcU16 count,CcStats *stats)
{return cast(a,delta,walls,count,stats,false);}
CcHit cc_trace(CcVec start,CcVec delta,const CcBox *walls,CcU16 count,CcStats *stats)
{CcBox point={start.x,start.y,0,0};return cast(point,delta,walls,count,stats,true);}
CcMove cc_slide(CcBox a,CcVec delta,const CcBox *walls,CcU16 count)
{
    CcMove m={a,CC_MISS,0,delta,{0,0},0,0};
    /* Run even a zero move so an invalid list or initial overlap is reported. */
    for(CcU16 step=0;step<CC_SLIDES;step++) {
        CcStats stats;CcHit h=cc_cast(m.box,m.remaining,walls,count,&stats);
        m.stats.visited+=stats.visited;m.stats.narrow+=stats.narrow;
        if(h.kind==CC_INVALID || h.kind==CC_OVERLAP){m.kind=h.kind;return m;}
        if(h.kind==CC_MISS) {
            m.box.x+=m.remaining.x;m.box.y+=m.remaining.y;m.remaining=(CcVec){0,0};return m;
        }
        m.kind=CC_HIT;m.contacts++;m.nx=h.nx;m.ny=h.ny;
        /* Use the exact fraction, not the rounded display time. Products fit CcS32. */
        CcS16 used_x=(CcS16)(quotient(product(m.remaining.x,h.num),h.den));
        CcS16 used_y=(CcS16)(quotient(product(m.remaining.y,h.num),h.den));
        m.box.x+=used_x;m.box.y+=used_y;
        m.remaining.x-=used_x;m.remaining.y-=used_y;
        /* A blocked coordinate is exactly on an integer raw-unit face. */
        if(h.nx)m.remaining.x=0;
        if(h.ny)m.remaining.y=0;
        if(!m.remaining.x && !m.remaining.y)return m;
    }
    m.kind=CC_EXHAUSTED;return m;
}

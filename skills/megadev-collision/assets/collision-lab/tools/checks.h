#ifndef COLLISION_CHECKS_H
#define COLLISION_CHECKS_H
#include "demo.h"
#include <stddef.h>
#include <string.h>
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)
static int check_sweep(void)
{
    CcBox a={0,0,16,16},b={80,0,16,16};CcVec zero={0,0};
    CcHit h=cc_sweep(a,(CcVec){128,0},b,zero);
    CHECK(h.kind==CC_HIT && h.time==2048 && h.nx==-1 && !h.ny);
    CHECK(h.num==64 && h.den==128);
    h=cc_sweep(b,(CcVec){-128,0},a,zero);CHECK(h.kind==CC_HIT && h.nx==1 && h.time==2048);
    h=cc_sweep(a,(CcVec){32,0},b,(CcVec){-96,0});CHECK(h.kind==CC_HIT && h.time==2048);
    CHECK(cc_sweep(a,zero,b,zero).kind==CC_MISS);
    CHECK(cc_sweep(a,(CcVec){128,0},(CcBox){80,16,16,16},zero).kind==CC_MISS);
    CHECK(cc_sweep(a,(CcVec){64,0},b,zero).kind==CC_HIT); /* End-face contact. */
    CHECK(!cc_overlap(a,(CcBox){16,0,16,16}));
    CHECK(cc_sweep(a,(CcVec){1,0},(CcBox){16,0,16,16},zero).time==0);
    CHECK(cc_sweep(a,(CcVec){-1,0},(CcBox){16,0,16,16},zero).kind==CC_MISS);
    CHECK(cc_sweep(a,(CcVec){0,1},(CcBox){16,0,16,16},zero).kind==CC_MISS);
    CHECK(cc_sweep(a,zero,(CcBox){8,8,16,16},zero).kind==CC_OVERLAP);
    h=cc_sweep(a,(CcVec){128,128},(CcBox){80,80,16,16},zero);
    CHECK(h.kind==CC_HIT && h.nx==-1 && h.ny==-1 && h.time==2048);
    CHECK(cc_sweep(a,(CcVec){128,128},(CcBox){80,48,16,16},zero).kind==CC_MISS); /* Grazes one instant. */
    h=cc_sweep((CcBox){-16000,-16000,4096,4096},(CcVec){4096,4096},(CcBox){-7808,-7808,4096,4096},zero);
    CHECK(h.kind==CC_HIT && h.time==4096 && h.nx==-1 && h.ny==-1);
    return 0;
}
static int check_ray(void)
{
    CcBox b={80,32,32,32};CcHit h=cc_ray((CcVec){0,48},(CcVec){160,0},b);
    CHECK(h.kind==CC_HIT && h.time==2048 && h.nx==-1);
    CHECK(cc_ray((CcVec){0,32},(CcVec){160,0},b).kind==CC_MISS); /* Along top edge. */
    CHECK(cc_ray((CcVec){96,48},(CcVec){0,0},b).kind==CC_OVERLAP);
    CHECK(cc_ray((CcVec){80,48},(CcVec){1,0},b).kind==CC_HIT);
    CHECK(cc_ray((CcVec){80,48},(CcVec){-1,0},b).kind==CC_MISS);
    CHECK(cc_ray((CcVec){112,48},(CcVec){-32,0},b).nx==1);
    CHECK(cc_ray((CcVec){0,48},(CcVec){0,0},b).kind==CC_MISS);
    return 0;
}
static int check_cast_slide(void)
{
    CcBox walls[]={{80,-64,16,256},{-64,80,256,16},{1000,1000,16,16}};
    CcBox a={0,0,16,16};CcStats stats;
    CcHit h=cc_cast(a,(CcVec){128,96},walls,3,&stats);
    CHECK(h.kind==CC_HIT && h.index==0 && h.nx==-1 && !h.ny && stats.visited==3 && stats.narrow==2);
    CcMove m=cc_slide(a,(CcVec){128,96},walls,3);
    CHECK(m.kind==CC_HIT && m.contacts==2 && m.box.x==64 && m.box.y==64);
    CHECK(!m.remaining.x && !m.remaining.y && m.stats.visited<=CC_MAX_BOXES*CC_SLIDES);
    for(CcU16 i=0;i<3;i++)CHECK(!cc_overlap(m.box,walls[i]));
    m=cc_slide(a,(CcVec){128,32},walls,3);
    CHECK(m.kind==CC_HIT && m.box.x==64 && m.box.y==32 && m.contacts==1);
    m=cc_slide(a,(CcVec){128,128},walls,3);
    CHECK(m.kind==CC_HIT && m.box.x==64 && m.box.y==64 && m.contacts==1);
    CcBox swapped[]={walls[1],walls[0],walls[2]};
    CcMove reverse=cc_slide(a,(CcVec){128,128},swapped,3);
    CHECK(reverse.box.x==m.box.x && reverse.box.y==m.box.y);
    m=cc_slide((CcBox){64,0,16,16},(CcVec){0,48},walls,3);
    CHECK(m.box.x==64 && m.box.y==48 && m.kind==CC_MISS);
    m=cc_slide((CcBox){70,0,16,16},(CcVec){-32,0},walls,3);
    CHECK(m.kind==CC_OVERLAP && m.box.x==70 && m.remaining.x==-32);
    m=cc_slide(a,(CcVec){100,40},NULL,0);CHECK(m.box.x==100 && m.box.y==40);
    /* Exact rational choice: both display times round down to the same value. */
    CcBox thin[]={{2001,-16,1,64},{2000,-16,1,64}};
    h=cc_cast((CcBox){0,0,1,1},(CcVec){4096,0},thin,2,&stats);
    CHECK(h.index==1 && h.num==1999);
    CcBox full[CC_MAX_BOXES];
    for(CcU16 i=0;i<CC_MAX_BOXES;i++)full[i]=(CcBox){(CcS16)(1000+i*16),1000,16,16};
    h=cc_cast(a,(CcVec){128,0},full,CC_MAX_BOXES,&stats);
    CHECK(h.kind==CC_MISS && stats.visited==CC_MAX_BOXES && !stats.narrow);
    full[31]=walls[0];h=cc_cast(a,(CcVec){128,0},full,CC_MAX_BOXES,&stats);
    CHECK(h.kind==CC_HIT && h.index==31 && stats.visited==CC_MAX_BOXES && stats.narrow==1);
    return 0;
}
static int check_invalid(void)
{
    CcBox good={0,0,16,16};CcVec zero={0,0};CcStats stats;
    CHECK(cc_sweep(good,(CcVec){4097,0},good,zero).kind==CC_INVALID);
    CHECK(cc_sweep((CcBox){16380,0,16,16},(CcVec){16,0},good,zero).kind==CC_INVALID);
    CHECK(cc_sweep((CcBox){0,0,0,16},zero,good,zero).kind==CC_INVALID);
    CHECK(cc_sweep((CcBox){0,0,4097,16},zero,good,zero).kind==CC_INVALID);
    CHECK(cc_ray((CcVec){-16385,0},zero,good).kind==CC_INVALID);
    CHECK(cc_cast(good,zero,NULL,1,&stats).kind==CC_INVALID && !stats.visited);
    CHECK(cc_cast(good,zero,&good,33,&stats).kind==CC_INVALID);
    CcBox bad[]={{1000,1000,16,16},{16000,16000,-1,16}};
    CHECK(cc_cast(good,zero,bad,2,&stats).kind==CC_INVALID && !stats.visited);
    CHECK(cc_slide(good,zero,bad,2).kind==CC_INVALID);
    return 0;
}
static int check_demo(void) {
 CcDemo a,b;cd_init(&a,2);cd_init(&b,2);
 for(CcU16 i=0;i<60;i++)cd_advance(&a,5,CD_RIGHT,0);
 for(CcU16 i=0;i<50;i++)cd_advance(&b,6,CD_RIGHT,0);
 CHECK(a.player.x==b.player.x && a.player.y==b.player.y && a.tick==b.tick);
 cd_advance(&a,5,0,CD_START);CcU16 tick=a.tick;
 for(CcU16 i=0;i<50;i++)cd_advance(&a,6,CD_DOWN,0);
 CHECK(a.tick==tick);
 cd_init(&a,0);cd_tick(&a,0);
 CHECK(!a.discrete_hit && a.hit.kind==CC_HIT && a.swept.x==158*16);
 cd_init(&a,0);cd_advance(&a,5,0,CD_UP);
 CHECK(a.raised && a.hit.kind==CC_MISS);
 cd_init(&a,2);CHECK(cd_advance(&a,65535,CD_RIGHT,0)==4 && a.dropped==1);
 for(CcU16 i=0;i<3000;i++) {
  cd_tick(&a,(i/100)%2?CD_RIGHT|CD_DOWN:CD_LEFT|CD_UP);
  CcU16 n;const CcBox *walls=cd_walls(cd_map(&a),&n);
  CHECK(a.hit.kind!=CC_INVALID && a.hit.kind!=CC_OVERLAP && a.hit.kind!=CC_EXHAUSTED);
  for(CcU16 j=0;j<n;j++)CHECK(!cc_overlap(a.player,walls[j]));
 }
 CcStats stats;CcBox walls[]={{80,0,16,64},{40,0,16,64}};
 CcHit h=cc_trace((CcVec){0,32},(CcVec){128,0},walls,2,&stats);
 CHECK(h.kind==CC_HIT && h.index==1 && h.num==40);
 return 0;
}
#endif

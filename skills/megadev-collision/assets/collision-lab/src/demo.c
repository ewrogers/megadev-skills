#include "demo.h"
#include "../build/layouts.h"
CcBox const *cd_walls(CcU16 map,CcU16 *count) {
 if(!count)return 0;
 switch(map) {
  case 0:*count=sizeof(cd_layout_0)/sizeof(CcBox);return cd_layout_0;
  case 1:*count=sizeof(cd_layout_1)/sizeof(CcBox);return cd_layout_1;
  case 2:*count=sizeof(cd_layout_2)/sizeof(CcBox);return cd_layout_2;
  case 3:*count=sizeof(cd_layout_3)/sizeof(CcBox);return cd_layout_3;
  default:*count=0;return 0;
 }
}
CcU16 cd_map(CcDemo const *s){return s->mode==0?s->raised:(CcU16)(s->mode+1u);}
void cd_reset(CcDemo *s) {
 s->discrete=(CcBox){640,1616,96,96};s->swept=(CcBox){640,2640,96,96};
 s->player=(CcBox){768,1152,192,192};s->aim=(CcVec){4480,1600};
 s->hit=(CcHit){CC_MISS,0,65535,0,0,0,1};s->stats=(CcStats){0,0};
 s->discrete_hit=s->discrete_done=s->swept_done=s->contacts=0;
 s->clock=0;s->events=CD_EVENT_FIRE;
}
void cd_init(CcDemo *s,CcU16 mode) {
 s->mode=mode<3?mode:0;s->speed=2;s->raised=s->paused=s->clock=s->tick=s->dropped=0;cd_reset(s);
}
CcS16 cd_fraction(CcS16 d,CcS32 num,CcS32 den) {
 /* Query fractions satisfy 0<=num<=den<=8192; callers use validated displacements. */
#ifdef __m68k__
 CcU32 value=(CcU16)(d<0?-d:d);CcU16 factor=(CcU16)num,divisor=(CcU16)den;
 __asm__("mulu.w %1,%0\n\tdivu.w %2,%0":"+&d"(value):"d"(factor),"d"(divisor):"cc");
 CcS16 q=(CcU16)value;return d<0?(CcS16)-q:q;
#else
 return (CcS16)((CcS32)d*num/den);
#endif
}
void cd_tick(CcDemo *s,CcU16 held) {
 s->tick++;
 CcU16 count;CcBox const *walls=cd_walls(cd_map(s),&count);
 if(s->mode==0) {
  CcS16 speed=s->speed==0?128:s->speed==1?1024:2048;
  if(!s->discrete_done) {
   CcS16 d=speed;
 if(d>4864-s->discrete.x)d=(CcS16)(4864-s->discrete.x);
   s->discrete.x=(CcS16)(s->discrete.x+d);
   if(cc_overlap(s->discrete,walls[0])){s->discrete_hit=1;s->discrete_done=1;}
   if(s->discrete.x>=4864)s->discrete_done=1;
  }
  if(!s->swept_done) {
   CcS16 d=speed;
 if(d>4864-s->swept.x)d=(CcS16)(4864-s->swept.x);
   CcVec move={d,0};s->hit=cc_cast(s->swept,move,walls,count,&s->stats);
   if(s->hit.kind==CC_HIT) {
    s->swept.x=(CcS16)(s->swept.x+cd_fraction(d,s->hit.num,s->hit.den));s->swept_done=1;s->events|=CD_EVENT_HIT;
   } else if(s->hit.kind==CC_MISS)s->swept.x=(CcS16)(s->swept.x+d);
   else s->swept_done=1;
   if(s->swept.x>=4864)s->swept_done=1;
  }
 } else if(s->mode==1) {
  if((held&12)==CD_LEFT && s->aim.x>384)s->aim.x-=48;
  if((held&12)==CD_RIGHT && s->aim.x<4864)s->aim.x+=48;
  if((held&3)==CD_UP && s->aim.y>768)s->aim.y-=48;
  if((held&3)==CD_DOWN && s->aim.y<3328)s->aim.y+=48;
  if(s->aim.x<384)s->aim.x=384;
 if(s->aim.x>4864)s->aim.x=4864;
  if(s->aim.y<768)s->aim.y=768;
 if(s->aim.y>3328)s->aim.y=3328;
  s->hit=cc_trace((CcVec){768,2304},(CcVec){(CcS16)(s->aim.x-768),(CcS16)(s->aim.y-2304)},walls,count,&s->stats);
 } else {
  CcS16 speed=s->speed==0?32:s->speed==1?128:512;CcVec d={0,0};
  if((held&12)==CD_LEFT)d.x=(CcS16)-speed;
 if((held&12)==CD_RIGHT)d.x=speed;
  if((held&3)==CD_UP)d.y=(CcS16)-speed;
 if((held&3)==CD_DOWN)d.y=speed;
  CcMove m=cc_slide(s->player,d,walls,count);s->player=m.box;s->contacts=m.contacts;s->stats=m.stats;
  s->hit=(CcHit){m.kind,0,65535,m.nx,m.ny,0,1};
 }
}
CcU16 cd_advance(CcDemo *s,CcU16 elapsed,CcU16 held,CcU16 hit) {
 s->events=0;
 if(hit&CD_START){s->paused^=1;s->clock=0;return 0;}
 if(hit&CD_C){s->mode=(CcU16)(s->mode+1u);
 if(s->mode==3)s->mode=0;
 cd_reset(s);}
 if(s->paused)return 0;
 if(s->mode!=1 && (hit&CD_B)){s->speed++;
 if(s->speed==3)s->speed=0;
 cd_reset(s);}
 if(s->mode==0 && (hit&CD_UP)){s->raised^=1;cd_reset(s);}
 if(hit&CD_A)cd_reset(s);
 if(elapsed>20){elapsed=20;s->dropped++;}
 s->clock=(CcU16)(s->clock+elapsed);CcU16 steps=0;
 while(s->clock>=5){s->clock-=5;cd_tick(s,held);steps++;}
 return steps;
}

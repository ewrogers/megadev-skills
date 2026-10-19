#include "math_core.h"
#include "motion.h"
#include <stdio.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"check failed at %d\n",__LINE__);return 1;}} while(0)
int main(void) {
 MmS16 out=1234;MmU16 t=999;MmS32 dot=17;MmVec a={1,1},b={0,0};
 CHECK(mm_from_int(-128,&out) && out==-32768);
 CHECK(!mm_from_int(128,&out) && out==-32768);
 CHECK(mm_mul(-129,128,&out) && out==-64);
 CHECK(mm_div(-32768,256,&out) && out==-32768);
 CHECK(!mm_div(-32768,-256,&out) && out==-32768);
 CHECK(!mm_div(1,0,&out));CHECK(!mm_div(1,1,0));
 CHECK(!mm_add(32767,1,&out));CHECK(!mm_sub(-32768,1,&out));
 CHECK(mm_normalize(&a,&a) && a.x==181 && a.y==181);
 CHECK(!mm_normalize(&b,&a) && a.x==181);
 a=(MmVec){-32768,-32768};CHECK(mm_length2(&a)==2147483648u);
 CHECK(!mm_dot(&a,&a,&dot) && dot==17);
 CHECK(mm_isqrt(4294967295u)==65535);
 CHECK(mm_sin(0)==0 && mm_sin(256)==256 && mm_cos(512)==-256);
 for(MmU16 i=0;i<=256;i++) {
  CHECK(mm_lerp(-32768,32767,i,&out));CHECK(out==(MmS16)(-32768+(65535u*i)/256u));
  CHECK(mm_smoothstep(i,&t) && t<=256);
 }
 CHECK(!mm_smoothstep(257,&t) && t==256);
 MmGame ntsc,pal;mt_init(&ntsc);mt_init(&pal);ntsc.view=pal.view=1;
 for(int i=0;i<60;i++)mt_advance(&ntsc,5,0,0);
 for(int i=0;i<50;i++)mt_advance(&pal,6,0,0);
 CHECK(ntsc.ship.x==pal.ship.x && ntsc.ship.y==pal.ship.y && ntsc.tick==60 && !pal.faults);
 mt_advance(&pal,6,0,128);MmU16 tick=pal.tick;mt_advance(&pal,300,0,0);CHECK(pal.tick==tick);
 mt_init(&pal);CHECK(mt_advance(&pal,65535,0,0)==4 && pal.dropped==1);
 mt_init(&pal);mt_advance(&pal,5,0,0);MmU16 phase=pal.angle;
 mt_advance(&pal,5,0,64);CHECK(pal.angle==(MmU16)((phase+1024-2)&1023));
 pal.view=2;pal.t=100;pal.reverse=0;mt_advance(&pal,5,0,64);CHECK(pal.t==98);
 mt_init(&pal);pal.view=1;for(int i=0;i<600;i++)mt_advance(&pal,5,8,0);
 CHECK(pal.target.x==56*256 && !pal.faults);
 CHECK(!mm_normalize(0,&a) && !mm_normalize(&a,0));
 CHECK(!mm_rotate(0,0,&a) && !mm_rotate(&a,0,0));
 CHECK(!mm_dot(0,&a,&dot) && !mm_dot(&a,0,&dot) && !mm_dot(&a,&a,0));
 return 0;
}

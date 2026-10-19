#include "motion.h"
void mt_init(MmGame *s) {
 s->target=(MmVec){48*256,0};s->ship=(MmVec){-48*256,0};
 s->view=s->angle=s->tick=s->clock=s->paused=s->t=s->reverse=s->faults=s->dropped=0;s->speed=1;s->automatic=1;
}
void mt_tick(MmGame *s,MmU16 held) {
 s->tick++;MmU16 increment=(MmU16)(1u<<s->speed);
 s->angle=(MmU16)((s->angle+((s->view==0 && !s->automatic)?1024u-increment:increment))&1023u);
 if(!s->reverse){if(s->t>=256-increment){s->t=256;s->reverse=1;}else s->t=(MmU16)(s->t+increment);}
 else if(s->t<=increment){s->t=0;s->reverse=0;}else s->t=(MmU16)(s->t-increment);
 if(s->view!=1)return;
 if(held&15u)s->automatic=0;
 if(s->automatic) {
  if(!mm_mul(mm_cos(s->angle),48*256,&s->target.x) || !mm_mul(mm_sin((MmU16)(s->angle*2u)),32*256,&s->target.y))s->faults++;
 } else {
  if((held&12)==4 && s->target.x>-56*256)s->target.x-=128;
  if((held&12)==8 && s->target.x<56*256)s->target.x+=128;
  if((held&3)==1 && s->target.y>-40*256)s->target.y-=128;
  if((held&3)==2 && s->target.y<40*256)s->target.y+=128;
 }
 MmVec d,n;
 if(!mm_sub(s->target.x,s->ship.x,&d.x) || !mm_sub(s->target.y,s->ship.y,&d.y)){s->faults++;return;}
 if(mm_length2(&d)<256u*256u){s->ship=s->target;return;}
 if(!mm_normalize(&d,&n)){s->faults++;return;}
 MmS16 dx,dy;MmS16 step=(MmS16)(128u<<s->speed);
 if(!mm_mul(n.x,step,&dx) || !mm_mul(n.y,step,&dy) || !mm_add(s->ship.x,dx,&s->ship.x) || !mm_add(s->ship.y,dy,&s->ship.y))s->faults++;
}
MmU16 mt_advance(MmGame *s,MmU16 elapsed,MmU16 held,MmU16 hit) {
 if(hit&128){s->paused^=1;s->clock=0;return 0;}
 if(s->paused)return 0;
 if(hit&16){MmU16 view=s->view;mt_init(s);s->view=view;}
 if(hit&32){s->view++;if(s->view==3)s->view=0;}
 if(hit&64){if(s->view==2)s->reverse^=1;else s->automatic^=1;}
 if(s->view!=1){if((hit&1) && s->speed<2)s->speed++;if((hit&2) && s->speed)s->speed--;}
 if(elapsed>20){elapsed=20;s->dropped++;}
 s->clock=(MmU16)(s->clock+elapsed);MmU16 count=0;
 while(s->clock>=5){s->clock-=5;mt_tick(s,held);count++;}
 return count;
}

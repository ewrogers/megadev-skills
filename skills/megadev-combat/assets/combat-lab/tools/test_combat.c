#include "combat.h"
#define CHECK(x) do {if(!(x))return __LINE__;}while(0)
static CoWorld a,b;
static void setup(void){
 co_init(&a);a.help=0;
 for(CoU16 i=0;i<CO_ACTORS;i++){a.actor[i].joined=0;a.actor[i].hp=5;a.actor[i].state=CO_IDLE;}
 a.actor[0].joined=a.actor[2].joined=1;
 a.actor[0].x=80;a.actor[0].y=64;a.actor[0].face=3;
 a.actor[2].x=104;a.actor[2].y=64;a.actor[2].face=2;
}
int combat_contract(void){
 CoBox x={0,0,8,8},y={8,0,8,8};CHECK(!co_overlap(&x,&y));y.x=7;CHECK(co_overlap(&x,&y));x.w=0;CHECK(!co_overlap(&x,&y));
 setup();CoActor *p=&a.actor[0],*e=&a.actor[2];CoBox box;
 co_swing(p);p->age=5;CHECK(!co_attack_box(p,&box));p->age=6;CHECK(co_attack_box(p,&box));
 co_resolve(&a);CHECK(e->hp==4 && a.hits==1);e->iframes=0;co_resolve(&a);CHECK(e->hp==4 && a.hits==1);
 p->age=13;CHECK(!co_attack_box(p,&box));p->state=CO_IDLE;co_swing(p);p->age=6;co_resolve(&a);CHECK(e->hp==3);
 setup();p=&a.actor[0];e=&a.actor[2];e->guard=1;co_swing(p);p->age=6;co_resolve(&a);CHECK(e->hp==5 && a.blocks==1);
 e->face=3;p->state=CO_IDLE;co_swing(p);p->age=6;co_resolve(&a);CHECK(e->hp==4); /* rear attack */
 setup();p=&a.actor[0];e=&a.actor[2];co_swing(p);co_swing(e);p->age=e->age=6;
 co_resolve(&a);CHECK(p->hp==4 && e->hp==4 && a.hits==2); /* simultaneous trade */
 setup();p=&a.actor[0];e=&a.actor[2];co_swing(p);p->age=6;p->state=CO_HURT;co_resolve(&a);CHECK(e->hp==5);
 setup();p=&a.actor[0];e=&a.actor[2];e->iframes=1;co_swing(p);p->age=6;co_resolve(&a);e->iframes=0;co_resolve(&a);CHECK(e->hp==5);
 setup();e=&a.actor[2];e->guard=1;
 a.bolt[0].x=104;a.bolt[0].y=60;a.bolt[0].dx=4;a.bolt[0].life=10;a.bolt[0].owner=0;a.bolt[0].generation=1;
 co_resolve(&a);CHECK(!a.bolt[0].life && e->hp==5 && a.blocks==1);
 a.bolt[0].life=10;a.bolt[0].generation=0;co_resolve(&a);CHECK(!a.bolt[0].life && e->hp==5);
 a.bolt[0].life=10;a.bolt[0].owner=65535;co_resolve(&a);CHECK(!a.bolt[0].life);
 /* Two simultaneous sources coalesce; the next step respects invulnerability. */
 setup();a.actor[1].joined=1;a.actor[1].x=80;a.actor[1].y=64;a.actor[1].face=3;
 co_swing(&a.actor[0]);co_swing(&a.actor[1]);a.actor[0].age=a.actor[1].age=6;
 co_resolve(&a);CHECK(a.actor[2].hp==4 && a.hits==1 && a.actor[1].hp==5);
 /* Full bolt pool refuses a cast without replacing any live projectile. */
 setup();for(CoU16 i=0;i<CO_BOLTS;i++){a.bolt[i].life=50;a.bolt[i].owner=0;a.bolt[i].generation=1;a.bolt[i].x=48;a.bolt[i].y=144;}
 co_tick(&a,0,0,CO_FIRE,0);CHECK(!(a.events&CO_SHOOT) && a.actor[0].cooldown==0);
 setup();p=&a.actor[0];co_swing(p);p->serial=65535;p->state=CO_IDLE;co_swing(p);CHECK(p->serial==0 && !p->victims);
 co_init(&a);co_init(&b);a.help=b.help=0;
 for(CoU16 i=0;i<60;i++)co_advance(&a,5,CO_RIGHT,0,0,0);
 for(CoU16 i=0;i<50;i++)co_advance(&b,6,CO_RIGHT,0,0,0);
 for(CoU16 i=0;i<sizeof(a);i++)CHECK(((CoU8 *)&a)[i]==((CoU8 *)&b)[i]);
 CHECK(a.tick==60);co_advance(&a,5,0,0,CO_START,0);CoU16 t=a.tick;
 co_advance(&a,65535,CO_RIGHT,0,CO_ATTACK,0);CHECK(a.tick==t && a.paused);
 co_advance(&a,5,0,0,CO_START,0);CHECK(co_advance(&a,65535,0,0,0,0)==4 && a.clamped==1);
 co_init(&a);a.help=0;CHECK(co_advance(&a,1,0,0,CO_ATTACK,0)==0);co_advance(&a,4,0,0,0,0);CHECK(a.actor[0].state==CO_SWING);
 co_init(&a);a.help=0;co_advance(&a,5,0,0,0,CO_START);CHECK(a.actor[1].joined && a.actor[1].iframes>0);
 co_init(&a);a.help=0;a.actor[0].x=298;a.actor[0].y=171;
 co_advance(&a,5,0,0,0,CO_START);CHECK(a.actor[1].joined && !co_blocked(a.actor[1].x,a.actor[1].y));
 setup();p=&a.actor[0];p->state=CO_HURT;p->kx=-2;p->x=21;
 co_tick(&a,0,0,0,0);CHECK(p->x==21); /* knockback respects walls */
 co_init(&a);a.help=0;
 for(CoU16 i=0;i<1800;i++){
  co_advance(&a,5,(i&64u)?CO_RIGHT:CO_LEFT,0,(i&31u)==0?CO_ATTACK:0,0);
  CHECK(a.candidates<=52 && a.work_high<=52);
  for(CoU16 n=0;n<CO_ACTORS;n++)CHECK(!co_blocked(a.actor[n].x,a.actor[n].y));
 }
 return 0;
}
#ifndef COMBAT_TARGET_PROBE
int main(void){return combat_contract();}
#endif

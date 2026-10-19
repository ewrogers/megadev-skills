#define COMBAT_TARGET_PROBE
#include "../tools/test_combat.c"
extern CoWorld world;
void combat_win_probe(void){world.help=0;world.complete=1;}
void combat_begin_probe(void){co_init(&world);world.help=0;}
void combat_guard_probe(CoU16 face){co_init(&world);world.help=0;world.actor[0].guard=1;world.actor[0].face=face;}
void combat_dense_probe(void){
 co_init(&world);world.help=0;
 for(CoU16 i=0;i<CO_ACTORS;i++){
  CoActor *a=&world.actor[i];a->joined=1;a->hp=5;a->state=CO_SWING;a->age=8;a->face=3;a->x=(CoS16)(40+i*48);a->y=80;a->flash=8;
 }
 for(CoU16 i=0;i<CO_BOLTS;i++){world.bolt[i].life=10;world.bolt[i].x=(CoS16)(32+i*32);world.bolt[i].y=80;}
}

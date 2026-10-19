#define COMBAT_TARGET_PROBE
#include "../tools/test_combat.c"
extern CoWorld world;
void combat_win_probe(void){world.help=0;world.complete=1;}
void combat_begin_probe(void){co_init(&world);world.help=0;}
void combat_guard_probe(CoU16 face){co_init(&world);world.help=0;world.actor[0].guard=1;world.actor[0].face=face;}

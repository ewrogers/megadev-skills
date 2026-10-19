#define ENEMY_TARGET_PROBE
#include "../tools/test_enemy.c"

extern AiWorld world;
void enemy_win_probe(void){world.complete=1;}

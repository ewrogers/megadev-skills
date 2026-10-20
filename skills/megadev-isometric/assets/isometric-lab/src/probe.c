#define ISOMETRIC_TARGET_PROBE
#include "../tools/test_isometric.c"
extern IsoWorld world;
void isometric_begin_probe(void){iso_world_init(&world);}
void isometric_tour_probe(IsoU16 stop){iso_tour(&world.game,stop);}
void isometric_cutaway_probe(void){world.cutaway^=1;}
void isometric_info_probe(void){world.info=1;}

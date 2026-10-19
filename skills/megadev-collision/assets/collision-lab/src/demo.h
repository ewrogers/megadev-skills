#ifndef MEGADEV_COLLISION_DEMO_H
#define MEGADEV_COLLISION_DEMO_H
#include "collision.h"
#define CD_A 64
#define CD_B 16
#define CD_C 32
#define CD_START 128
#define CD_UP 1
#define CD_DOWN 2
#define CD_LEFT 4
#define CD_RIGHT 8
#define CD_EVENT_HIT 1
#define CD_EVENT_FIRE 2
typedef struct {
 CcBox discrete,swept,player;
 CcVec aim;
 CcHit hit;
 CcStats stats;
 CcU16 mode,speed,raised,paused,clock,tick,discrete_hit,discrete_done,swept_done,events,dropped,contacts;
} CcDemo;
void cd_init(CcDemo *s,CcU16 mode);
void cd_reset(CcDemo *s);
CcU16 cd_advance(CcDemo *s,CcU16 elapsed300,CcU16 held,CcU16 pressed);
void cd_tick(CcDemo *s,CcU16 held);
CcBox const *cd_walls(CcU16 map,CcU16 *count);
CcU16 cd_map(CcDemo const *s);
CcS16 cd_fraction(CcS16 d,CcS32 num,CcS32 den);
/* Pointer-only test/interop seams use the same production query implementations. */
void cp_sweep(CcBox const *a,CcVec const *da,CcBox const *b,CcVec const *db,CcHit *out);
void cp_ray(CcVec const *p,CcVec const *d,CcBox const *b,CcHit *out);
void cp_slide(CcBox const *a,CcVec const *d,CcBox const *walls,CcU16 count,CcMove *out);
#endif

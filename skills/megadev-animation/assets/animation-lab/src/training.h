#ifndef ANIMATION_LAB_TRAINING_H
#define ANIMATION_LAB_TRAINING_H
#include "animation.h"
#define TR_UP 1
#define TR_DOWN 2
#define TR_LEFT 4
#define TR_RIGHT 8
#define TR_B 16
#define TR_C 32
#define TR_A 64
#define TR_START 128
#define TR_EVENT_HIT 4
#define TR_EVENT_HURT 8
#define TR_EVENT_REVIVE 16
#define TR_EVENT_DONE 32
typedef struct {
 AnPlayer player;
 AnS16 x; /* Quarter pixels; flat practice strip has feet at y=144. */
 AnU16 left,clock,slow_clock,tick,paused,slow,debug,pending;
 AnU16 hits,contacts,hit_latched,dummy_flash,events,steps,swishes,completions,interrupts,dropped;
} Training;
void tr_init(Training *s);
AnU16 tr_advance(Training *s,AnU16 elapsed300,AnU16 held,AnU16 pressed);
void tr_tick(Training *s,AnU16 held,AnU16 pressed);
#endif

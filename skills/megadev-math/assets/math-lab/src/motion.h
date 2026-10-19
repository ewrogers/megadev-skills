#ifndef MM_MOTION_H
#define MM_MOTION_H
#include "math_core.h"
typedef struct {
 MmVec target,ship;
 MmU16 view,speed,angle,tick,clock,paused,t,reverse,automatic,faults,dropped;
} MmGame;
void mt_init(MmGame *s);
MmU16 mt_advance(MmGame *s,MmU16 elapsed300,MmU16 held,MmU16 pressed);
void mt_tick(MmGame *s,MmU16 held);
#endif

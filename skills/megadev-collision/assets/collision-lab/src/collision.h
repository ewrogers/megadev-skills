#ifndef MEGADEV_COLLISION_H
#define MEGADEV_COLLISION_H
#include <stdbool.h>
typedef signed short CcS16;
typedef unsigned short CcU16;
typedef signed int CcS32;
typedef unsigned int CcU32;
_Static_assert(sizeof(CcS16)==2 && sizeof(CcS32)==4,"Collision requires 16/32-bit integers");
/* Local Q4 coordinates, not an SDK fixed-point format. 16 units = one pixel. */
#define CC_PIXEL 16
#define CC_TIME 4096
#define CC_LIMIT 16384
#define CC_EXTENT 4096
#define CC_MOTION 4096
#define CC_MAX_BOXES 32
#define CC_SLIDES 3
enum {CC_MISS,CC_HIT,CC_OVERLAP,CC_INVALID,CC_EXHAUSTED};
typedef struct {CcS16 x,y;} CcVec;
typedef struct {CcS16 x,y,w,h;} CcBox;
typedef struct {CcU16 kind,time,index;CcS16 nx,ny;CcS32 num,den;} CcHit;
typedef struct {CcU16 visited,narrow;} CcStats;
typedef struct {CcBox box;CcU16 kind,contacts;CcVec remaining;CcStats stats;CcS16 nx,ny;} CcMove;

bool cc_overlap(CcBox a,CcBox b);
CcHit cc_sweep(CcBox a,CcVec da,CcBox b,CcVec db);
CcHit cc_ray(CcVec start,CcVec delta,CcBox target);
CcHit cc_cast(CcBox a,CcVec delta,const CcBox *walls,CcU16 count,CcStats *stats);
CcHit cc_trace(CcVec start,CcVec delta,const CcBox *walls,CcU16 count,CcStats *stats);
CcMove cc_slide(CcBox a,CcVec delta,const CcBox *walls,CcU16 count);
#endif

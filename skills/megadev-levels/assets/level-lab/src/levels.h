#ifndef MEGADEV_LEVELS_H
#define MEGADEV_LEVELS_H
/* Application-owned little tile world; not an SDK map API. */
typedef unsigned char LvU8;
typedef unsigned short LvU16;
typedef signed short LvS16;
typedef unsigned int LvU32;
_Static_assert(sizeof(LvU16)==2 && sizeof(LvU32)==4,"Level integer ABI changed");
#if defined(_WIN32) && defined(LEVELS_HOST_DLL)
#define LV_API __declspec(dllexport)
#else
#define LV_API
#endif
#define LV_AREAS 4
#define LV_RUNS 4
#define LV_WORDS 192
#define LV_UP 1
#define LV_DOWN 2
#define LV_LEFT 4
#define LV_RIGHT 8
#define LV_USE 64
#define LV_DEBUG 16
#define LV_TOUR 32
#define LV_PAUSE 128
#define LV_EVENT_TRAVEL 1
#define LV_EVENT_WARP 2
#define LV_EVENT_NO 4
typedef struct {LvU8 const *data;LvU16 w,h,tiles,spawns,exits;} LvArea;
typedef struct {LvArea areas[LV_AREAS];LvU16 count;} LvWorld;
typedef struct {
 LvU16 area,x,y,camera_x,camera_y,face,moving,tick,paused,clock,pending;
 LvU16 debug,waypoint,near,visits,events,dropped;
} LvGame;
typedef struct {LvU16 address,count,step,offset;} LvRun;
typedef struct {LvRun runs[LV_RUNS];LvU16 words[LV_WORDS];LvU16 count,used;} LvPatch;
LV_API LvU16 lv_open(LvWorld *world,LvU8 const *data,LvU32 bytes);
LV_API LvU16 lv_tile(LvArea const *a,LvU16 plane,LvS16 tx,LvS16 ty);
LV_API LvU16 lv_solid(LvArea const *a,LvS16 x,LvS16 y);
LV_API LvU16 lv_blocked(LvArea const *a,LvS16 x,LvS16 y);
LV_API LvU16 lv_spawn(LvWorld const *w,LvGame *s,LvU16 area,LvU16 spawn);
LV_API void lv_camera(LvWorld const *w,LvGame *s);
LV_API void lv_init(LvWorld const *w,LvGame *s);
LV_API LvU16 lv_advance(LvWorld const *w,LvGame *s,LvU16 elapsed300,LvU16 held,LvU16 pressed);
LV_API LvU16 lv_word(LvArea const *a,LvU8 const *attrs,LvU16 plane,LvU16 x8,LvU16 y8,LvU16 debug);
LV_API LvU16 lv_patch(LvPatch *p,LvArea const *a,LvU8 const *attrs,LvU16 old_x,LvU16 old_y,LvU16 x,LvU16 y,LvU16 debug);
#endif

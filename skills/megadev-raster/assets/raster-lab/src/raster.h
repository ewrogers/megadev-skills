#ifndef RASTER_H
#define RASTER_H
typedef unsigned short RtU;
typedef signed short RtS;
typedef unsigned int RtL;
#if defined(_WIN32) && defined(RASTER_HOST_DLL)
#define RT_API __declspec(dllexport)
#else
#define RT_API
#endif
#define RT_LINES 224
#define RT_SPRITES 18
/* Assembly contract: ten contiguous words, no pointers into mutable models. */
typedef struct {RtU line,flags,scroll_a,scroll_b,table,wet0,wet1,enabled,top_a,top_b;} RtPlan;
typedef struct {
 RtU x[2],y[2],line,tick,remainder,selected,enabled,waves,automatic,paused,demo,direction,clamped;
} RtWorld;
typedef struct {RtPlan plan;RtU scroll[RT_LINES][2];RtU sat[RT_SPRITES][4];RtU count;} RtView;
RT_API void rt_init(RtWorld *w,RtU demo);
RT_API RtU rt_advance(RtWorld *w,RtU units,RtU held,RtU hit);
RT_API void rt_build(RtWorld const *w,RtView *v);
RT_API RtU rt_plan_valid(RtPlan const *p);
void raster_install(void);
void raster_detach(void);
extern volatile RtU raster_epoch,raster_armed,raster_hits,raster_done,raster_late,raster_timeout,raster_reg_off;
extern volatile RtU raster_actual,raster_target,raster_setup;
extern volatile RtPlan raster_plan;
#endif

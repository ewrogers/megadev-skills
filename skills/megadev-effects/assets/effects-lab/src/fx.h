#ifndef FX_H
#define FX_H
typedef unsigned short FxU;
typedef signed short FxS;
#define FX_POOL 64
#define FX_SAT 56
#define FX_BANDS 28
/* Q12.4 positions/velocities. A zero life marks a free slot. */
typedef struct {FxS x,y,vx,vy;FxU life,kind;} FxParticle;
typedef struct {
 FxParticle p[FX_POOL];
 FxU x,y,tick,remainder,seed,weather,density,paused,shake,left,denied,clamped;
} FxWorld;
typedef struct {
 FxU sat[FX_SAT][4];
 FxU count,omitted,culled;FxS shake_x,shake_y;
 unsigned char bands[FX_BANDS];
} FxView;
void fx_init(FxWorld *w);
FxU fx_advance(FxWorld *w,FxU units,FxU held,FxU hit);
void fx_burst(FxWorld *w);
void fx_build(FxWorld const *w,FxView *v);
FxU fx_alive(FxWorld const *w,FxU first,FxU end);
#endif

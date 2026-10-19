#ifndef COMBAT_LAB_H
#define COMBAT_LAB_H
/* Application policy, not SDK APIs. All positions are bounded native pixels. */
typedef unsigned char CoU8;
typedef unsigned short CoU16;
typedef signed short CoS16;
#define CO_ACTORS 5
#define CO_BOLTS 8
#define CO_UP 1u
#define CO_DOWN 2u
#define CO_LEFT 4u
#define CO_RIGHT 8u
#define CO_GUARD 16u
#define CO_FIRE 32u
#define CO_ATTACK 64u
#define CO_START 128u
enum {CO_IDLE,CO_SWING,CO_HURT,CO_DEAD};
enum {CO_SWISH=1,CO_HIT=2,CO_BLOCK=4,CO_DAMAGE=8,CO_SHOOT=16,CO_CLEAR=32};
typedef struct {CoS16 x,y,w,h;} CoBox;
typedef struct {
 CoS16 x,y,kx,ky;
 CoU16 face,state,age,hp,iframes,guard,flash,cooldown,serial,generation,joined,moving;
 CoU16 victims; /* One bounded bit per defender, reset for every new swing. */
} CoActor;
typedef struct {CoS16 x,y,dx,dy;CoU16 life,owner,generation;} CoBolt;
typedef struct {
 CoActor actor[CO_ACTORS];CoBolt bolt[CO_BOLTS];
 CoU16 tick,clock,pending[2],paused,help,complete,game_over,events,round,hits,blocks;
 CoU16 candidates,work_high,clamped,debug;
} CoWorld;
#if defined(_WIN32)
#define CO_API __declspec(dllexport)
#else
#define CO_API
#endif
CO_API CoU16 co_overlap(CoBox const *a,CoBox const *b);
CO_API CoU16 co_attack_box(CoActor const *a,CoBox *box);
CO_API CoU16 co_front(CoActor const *a,CoS16 x,CoS16 y);
CO_API CoU16 co_blocked(CoS16 x,CoS16 y);
CO_API void co_init(CoWorld *w);
CO_API void co_swing(CoActor *a);
CO_API void co_resolve(CoWorld *w);
CO_API void co_tick(CoWorld *w,CoU16 h1,CoU16 h2,CoU16 p1,CoU16 p2);
CO_API CoU16 co_advance(CoWorld *w,CoU16 elapsed300,CoU16 h1,CoU16 h2,CoU16 p1,CoU16 p2);
#endif

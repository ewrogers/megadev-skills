#ifndef TEST_REPLAY_H
#define TEST_REPLAY_H
/* Application-owned QA fixture; not Megadev APIs or a replay file ABI. */
typedef unsigned short Rp16;
typedef signed short RpS16;
typedef unsigned int Rp32;
#define RP_CAP 720u
#define RP_SCHEMA 1u
#define RP_CONTENT 0x4B31u
#define RP_UP 1u
#define RP_DOWN 2u
#define RP_LEFT 4u
#define RP_RIGHT 8u
#define RP_READY 1u
#define RP_FAILED 2u
enum { RP_LIVE,RP_RECORD,RP_PLAY,RP_MATCH,RP_DIVERGED,RP_FULL,RP_INVALID };
typedef struct { Rp16 x,y,tick,seed,beacons,hits,gate,goals,cooldown,hazard_y; } RpState;
typedef struct { Rp16 held,event;Rp32 hash; } RpSample;
typedef struct {
    RpState state,initial;
    RpSample tape[RP_CAP];
    Rp32 expected,actual;
    Rp16 length,cursor,mode,first_bad,schema,content,seed,clock_remainder;
} Replay;
void rp_reset_state(RpState *state,Rp16 seed);
Rp32 rp_hash(RpState const *state);
int rp_step(RpState *state,Rp16 held,Rp16 event,Rp16 bug);
void rp_init(Replay *r,Rp16 seed);
void rp_record(Replay *r);
void rp_stop(Replay *r);
int rp_play(Replay *r);
void rp_tick(Replay *r,Rp16 held,Rp16 event,Rp16 bug);
/* 300 Hz elapsed units -> 60 Hz simulation. Refuse overload; do not drop ticks. */
int rp_advance(Replay *r,Rp16 elapsed,Rp16 held,Rp16 bug);
Rp16 rp_fixture_event(Rp16 tick);
#endif

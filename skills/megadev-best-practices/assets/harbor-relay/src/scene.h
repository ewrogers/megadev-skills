#ifndef SCENE_H
#define SCENE_H
typedef unsigned short Sc16;
enum { SC_TITLE, SC_QUAY, SC_CAVERN, SC_OPTIONS, SC_CREDITS, SC_RESULTS, SC_COUNT };
enum { SC_RUN, SC_OUT, SC_DRAIN, SC_LOAD, SC_COMMIT, SC_IN };
enum { SC_BLACK=1, SC_RETIRED=2, SC_READY=4, SC_FAILED=8, SC_PUBLISHED=16 };
/* All clocks use 300 Hz units, independent of redraw rate. */
typedef struct {
    Sc16 current,target,queued,phase,fade,clock,age,stalled;
    Sc16 generation,leases,transitions,failures,room_value,paused;
} Scene;
void scene_init(Scene *s);
int scene_request(Scene *s,Sc16 next);
void scene_tick(Scene *s,Sc16 elapsed,Sc16 events,Sc16 cached);
int scene_hold(Scene *s,Sc16 generation);
int scene_release(Scene *s,Sc16 generation);
int scene_valid(Scene const *s,Sc16 generation);
int scene_pause(Scene *s);
int scene_world(Sc16 id);
#endif

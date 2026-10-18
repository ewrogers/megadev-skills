#include "scene.h"
#include <assert.h>
static void black(Scene *s) {
    scene_tick(s,120,0,0);assert(s->fade==0 && s->phase==SC_OUT);
    scene_tick(s,0,SC_BLACK,0);assert(s->phase==SC_DRAIN);
}
static void enter(Scene *s,Sc16 id) {
    assert(scene_request(s,id)==1);black(s);
    scene_tick(s,0,SC_RETIRED,1);assert(s->phase==SC_COMMIT);
    scene_tick(s,0,SC_PUBLISHED,1);scene_tick(s,120,0,1);assert(s->phase==SC_RUN && s->current==id);
}
int main(void) {
    Scene s;scene_init(&s);assert(!scene_request(&s,SC_COUNT));
    enter(&s,SC_QUAY);Sc16 old=s.generation;s.room_value=37;
    assert(scene_hold(&s,old));assert(scene_pause(&s));assert(scene_pause(&s));
    assert(scene_request(&s,SC_CAVERN));assert(!scene_hold(&s,old));black(&s);
    scene_tick(&s,9000,SC_RETIRED,1);assert(s.phase==SC_DRAIN && s.stalled && s.room_value==37);
    assert(!scene_release(&s,old-1));assert(scene_release(&s,old));assert(!scene_release(&s,old));
    scene_tick(&s,0,0,1);assert(s.phase==SC_DRAIN);
    scene_tick(&s,0,SC_RETIRED,0);assert(s.phase==SC_LOAD);
    scene_tick(&s,120,0,0);assert(s.current==SC_QUAY && s.room_value==37);
    scene_tick(&s,0,SC_READY,1);assert(s.phase==SC_COMMIT);
    scene_tick(&s,0,0,1);assert(s.current==SC_QUAY);
    scene_tick(&s,0,SC_PUBLISHED,1);assert(s.current==SC_CAVERN && !s.room_value && !scene_valid(&s,old));
    scene_tick(&s,120,0,1);assert(s.phase==SC_RUN && !s.stalled);
    /* Failure preserves the old scene, its progress and handle generation. */
    s.room_value=9;old=s.generation;assert(scene_request(&s,SC_QUAY));black(&s);
    scene_tick(&s,0,SC_RETIRED,0);scene_tick(&s,0,SC_FAILED,0);
    scene_tick(&s,0,SC_PUBLISHED,0);scene_tick(&s,120,0,0);
    assert(s.current==SC_CAVERN && s.room_value==9 && s.generation==old && s.failures==1);
    /* One latest request; don't restart a live transition or skip retirement. */
    assert(scene_request(&s,SC_TITLE)==1);assert(scene_request(&s,SC_OPTIONS)==2);
    assert(scene_request(&s,SC_CREDITS)==2);black(&s);
    scene_tick(&s,0,SC_RETIRED,1);scene_tick(&s,0,SC_PUBLISHED,1);scene_tick(&s,120,0,1);
    assert(s.phase==SC_OUT && s.target==SC_CREDITS && s.current==SC_TITLE);
    assert(!scene_pause(&s));
    /* Equal real elapsed time at PAL/NTSC; no redraw-count timing. */
    for(Sc16 hz=50;hz<=60;hz+=10) {
        scene_init(&s);assert(scene_request(&s,SC_OPTIONS));
        for(Sc16 i=0;i<hz*2/5;i++)scene_tick(&s,300/hz,0,0);
        assert(!s.fade && s.phase==SC_OUT);
    }
    scene_init(&s);s.generation=0xFFFF;assert(!scene_request(&s,SC_QUAY));
    return 0;
}

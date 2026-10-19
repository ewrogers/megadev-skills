#include "scene.h"
_Static_assert(sizeof(Sc16)==2,"16-bit scene words required");
int scene_world(Sc16 id){return id==SC_QUAY || id==SC_CAVERN;}
void scene_init(Scene *s){*s=(Scene){0};s->queued=SC_COUNT;s->fade=8;s->generation=1;}
int scene_request(Scene *s,Sc16 next) {
    if(next>=SC_COUNT)return 0;
    if(s->phase!=SC_RUN){s->queued=next;return 2;}
    if(next==s->current || s->generation==0xFFFFu)return 0;
    s->target=next;s->phase=SC_OUT;s->clock=0;s->age=0;s->stalled=0;s->paused=0;return 1;
}
void scene_tick(Scene *s,Sc16 elapsed,Sc16 events,Sc16 cached) {
    if(s->phase==SC_RUN)return;
    if(elapsed>9000)elapsed=9000;
    if(elapsed>=9000u-s->age){s->age=9000;s->stalled=1;}else s->age+=elapsed;
    if(s->phase==SC_OUT || s->phase==SC_IN) {
        /* At most eight fade steps are useful. Avoid implicit 32-bit division:
           Megadev links these bare modules without automatic libgcc helpers. */
        Sc16 total=(Sc16)(s->clock+(elapsed>120?120:elapsed)),steps=0;
        while(total>=15){total-=15;steps++;}s->clock=total;
        if(s->phase==SC_OUT){s->fade=steps>=s->fade?0:(Sc16)(s->fade-steps);
            if(!s->fade && (events&SC_BLACK))s->phase=SC_DRAIN;
        } else {
            s->fade=steps>=8u-s->fade?8:(Sc16)(s->fade+steps);
            if(s->fade==8){Sc16 next=s->queued;s->queued=SC_COUNT;s->phase=SC_RUN;s->age=0;s->stalled=0;
                if(next<SC_COUNT)scene_request(s,next);}
        }
    } else if(s->phase==SC_DRAIN) {
        if((events&SC_RETIRED) && !s->leases)s->phase=scene_world(s->target)&&!cached?SC_LOAD:SC_COMMIT;
    } else if(s->phase==SC_LOAD) {
        if(events&SC_FAILED){s->failures++;s->target=s->current;s->phase=SC_COMMIT;}
        else if(events&SC_READY)s->phase=SC_COMMIT;
    } else if(s->phase==SC_COMMIT && !s->leases && (events&SC_PUBLISHED)) {
        /* Cleanup only after old work retired and the new map was published. */
        if(s->target!=s->current){s->room_value=0;s->current=s->target;s->generation++;s->transitions++;}
        s->phase=SC_IN;s->clock=0;
    }
}
int scene_valid(Scene const *s,Sc16 generation){return generation && generation==s->generation && scene_world(s->current);}
int scene_hold(Scene *s,Sc16 generation){if(s->phase!=SC_RUN || !scene_valid(s,generation) || s->leases==0xFFFFu)return 0;s->leases++;return 1;}
int scene_release(Scene *s,Sc16 generation){if(!scene_valid(s,generation) || !s->leases)return 0;s->leases--;return 1;}
int scene_pause(Scene *s){if(s->phase!=SC_RUN || !scene_world(s->current))return 0;s->paused^=1;return 1;}

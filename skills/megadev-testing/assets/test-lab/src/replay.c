#include "replay.h"
_Static_assert(sizeof(Rp16)==2 && sizeof(Rp32)==4,"Replay needs explicit integer widths");
_Static_assert(sizeof(RpSample)==8,"Tape RAM budget changed");
_Static_assert(sizeof(Replay)==5824,"Replay RAM budget changed");
static Rp16 distance(Rp16 a,Rp16 b){return a>b?a-b:b-a;}
void rp_reset_state(RpState *s,Rp16 seed){*s=(RpState){32,112,0,seed?seed:1,0,0,0,0,0,80};}
/* Field-wise 32-bit polynomial mixing: no struct padding, pointers, rendered frames or VBlank counters. */
static Rp32 mix(Rp32 h,Rp16 value){h^=value;return (h<<5)+(h<<1)+h;}
Rp32 rp_hash(RpState const *s){
    Rp32 h=0x811C9DC5u;
    h=mix(h,s->x);h=mix(h,s->y);h=mix(h,s->tick);h=mix(h,s->seed);
    h=mix(h,s->beacons);h=mix(h,s->hits);h=mix(h,s->gate);h=mix(h,s->goals);
    h=mix(h,s->cooldown);return mix(h,s->hazard_y);
}
int rp_step(RpState *s,Rp16 held,Rp16 event,Rp16 bug){
    if((held&~15u) || event>RP_FAILED)return 0;
    if(event==RP_READY)s->gate=1;else if(event==RP_FAILED)s->gate=2;
    Rp16 speed=bug?3:2;
    if((held&(RP_LEFT|RP_RIGHT))==RP_LEFT)s->x=s->x>16+speed?s->x-speed:16;
    if((held&(RP_LEFT|RP_RIGHT))==RP_RIGHT)s->x=s->x<288-speed?s->x+speed:288;
    if((held&(RP_UP|RP_DOWN))==RP_UP)s->y=s->y>40+speed?s->y-speed:40;
    if((held&(RP_UP|RP_DOWN))==RP_DOWN)s->y=s->y<168-speed?s->y+speed:168;
    /* Game-local PRNG; visual/audio RNG must not consume it. */
    s->seed=(Rp16)((s->seed>>1)^((s->seed&1u)?0xB400u:0));
    Rp16 phase=s->tick&127u;
    s->hazard_y=(Rp16)(56u+(phase<64u?phase:127u-phase));
    for(Rp16 i=0;i<3;i++)if(distance(s->x,(Rp16)(80u+i*72u))<13 && distance(s->y,112)<13)s->beacons|=(Rp16)(1u<<i);
    if(s->cooldown)s->cooldown--;
    if(!s->cooldown && distance(s->x,160)<12 && distance(s->y,s->hazard_y)<12){s->hits++;s->cooldown=60;}
    if(s->x>=280 && s->gate==1 && s->beacons==7){s->goals++;s->x=32;s->beacons=0;}
    s->tick++;return 1;
}
void rp_init(Replay *r,Rp16 seed){
    r->seed=seed?seed:1;r->length=r->cursor=r->clock_remainder=0;r->mode=RP_LIVE;
    r->schema=RP_SCHEMA;r->content=RP_CONTENT;r->first_bad=65535;r->expected=r->actual=0;
    rp_reset_state(&r->state,r->seed);r->initial=r->state;
}
void rp_record(Replay *r){
    rp_init(r,r->seed);r->mode=RP_RECORD;
}
void rp_stop(Replay *r){if(r->mode==RP_RECORD)r->mode=RP_LIVE;}
int rp_play(Replay *r){
    if(!r->length || r->length>RP_CAP || r->schema!=RP_SCHEMA || r->content!=RP_CONTENT){r->mode=RP_INVALID;return 0;}
    /* Validate the complete tape before mutating simulation state. */
    for(Rp16 i=0;i<r->length;i++)if((r->tape[i].held&~15u) || r->tape[i].event>RP_FAILED){r->mode=RP_INVALID;return 0;}
    r->state=r->initial;r->cursor=r->clock_remainder=0;r->first_bad=65535;
    r->expected=r->actual=0;r->mode=RP_PLAY;return 1;
}
void rp_tick(Replay *r,Rp16 held,Rp16 event,Rp16 bug){
    if(r->mode==RP_MATCH || r->mode==RP_DIVERGED || r->mode==RP_FULL || r->mode==RP_INVALID)return;
    if(r->mode==RP_PLAY){held=r->tape[r->cursor].held;event=r->tape[r->cursor].event;}
    if(!rp_step(&r->state,held,event,bug)){r->mode=RP_INVALID;return;}
    Rp32 hash=rp_hash(&r->state);
    if(r->mode==RP_RECORD){
        r->tape[r->length]=(RpSample){held,event,hash};r->length++;
        if(r->length==RP_CAP)r->mode=RP_FULL;
    } else if(r->mode==RP_PLAY){
        r->expected=r->tape[r->cursor].hash;r->actual=hash;
        if(hash!=r->expected){r->first_bad=r->cursor;r->mode=RP_DIVERGED;return;}
        r->cursor++;if(r->cursor==r->length)r->mode=RP_MATCH;
    }
}
Rp16 rp_fixture_event(Rp16 tick){return tick==180?RP_READY:0;}
int rp_advance(Replay *r,Rp16 elapsed,Rp16 held,Rp16 bug){
    if(elapsed>30u)return 0; /* Explicit fixture overload policy: caller reports it. */
    r->clock_remainder+=elapsed;
    while(r->clock_remainder>=5u){
        r->clock_remainder-=5u;
        rp_tick(r,held,rp_fixture_event(r->state.tick),bug);
    }
    return 1;
}

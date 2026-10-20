#include "input.h"
void in_clear(InSample *s,InU16 status){s->kind=IN_NONE;s->status=status;s->held=0;s->dx=0;s->dy=0;}
static InU16 extra(InU8 n) {
    n=(InU8)(~n)&15;
    return (InU16)(((n&4)<<6)|((n&2)<<8)|((n&1)<<10)|((n&8)<<8));
}
int in_pad(InU8 const v[8],InSample *s) {
    in_clear(s,IN_ABSENT);
    if((v[1]&12)!=0)return 0;
    s->kind=IN_PAD3;s->status=IN_OK;
    s->held=(InU16)(~((v[0]&63)|((v[1]&48)<<2)))&255;
    if((v[5]&15)==0 && (v[7]&15)==15){s->kind=IN_PAD6;s->held|=extra(v[6]);}
    return 1;
}
static InS16 motion(InU16 raw,InU16 negative) {
    return raw && negative?(InS16)((InS16)raw-256):(InS16)raw;
}
int in_mouse(InU8 const n[6],InSample *s) {
    in_clear(s,IN_BAD_PACKET);
    for(InU16 i=0;i<6;i++)if(n[i]>15)return 0;
    s->kind=IN_MOUSE;
    if(n[0]&12){s->status=IN_OVERFLOW;return 0;}
    s->status=IN_OK;
    s->held=(InU16)(((n[1]&1)<<6)|((n[1]&2)<<4)|((n[1]&4)<<2)|((n[1]&8)<<4));
    s->dx=motion((InU16)((n[2]<<4)|n[3]),n[0]&1);
    s->dy=(InS16)-motion((InU16)((n[4]<<4)|n[5]),n[0]&2);
    return 1;
}
int in_team(InU8 const *n,InU16 length,InSample out[4]) {
    InSample staged[4];InU16 used=6;
    for(InU16 i=0;i<4;i++){in_clear(&out[i],IN_BAD_PACKET);in_clear(&staged[i],IN_ABSENT);}
    if(length<6 || length>30 || n[0] || n[1])return 0;
    for(InU16 i=0;i<length;i++)if(n[i]>15)return 0;
    for(InU16 i=0;i<4;i++) {
        InU16 kind=n[i+2],need=kind==0?2:(kind==1?3:(kind==2?6:0));
        if(kind!=0 && kind!=1 && kind!=2 && kind!=15)return 0;
        if(need>length-used)return 0;
        if(kind<=1) {
            staged[i].kind=kind?IN_PAD6:IN_PAD3;staged[i].status=IN_OK;
            InU16 buttons=(InU16)(~n[used+1])&15;
            staged[i].held=(InU16)(((InU16)~n[used]&15)|((buttons&3)<<4)|((buttons&12)<<4));
            if(kind)staged[i].held|=extra(n[used+2]);
        } else if(kind==2)in_mouse(n+used,&staged[i]);
        used+=need;
    }
    if(used!=length)return 0;
    for(InU16 i=0;i<4;i++)out[i]=staged[i];
    return 1;
}
void in_reset(InSlot *s) {
    s->held=s->pressed=s->released=s->repeat=s->last_dir=s->repeat_time=s->kind=s->initialized=s->player=0;s->x=160;s->y=200;
}
static InU16 direction(InU16 held) {
    InU16 d=held&15;
    if((d&3)==3)d&=(InU16)~3u;
    if((d&12)==12)d&=(InU16)~12u;
    return d;
}
void in_update(InSlot *s,InSample const *v,InU16 hz) {
    s->pressed=s->released=s->repeat=0;
    if(v->status!=IN_OK || v->kind==IN_NONE || v->kind==IN_UNKNOWN) {
        s->released=s->held;s->held=0;s->initialized=0;s->last_dir=0;s->repeat_time=0;s->kind=v->kind;return;
    }
    InU16 d=direction(v->held);
    if(!s->initialized || s->kind!=v->kind) {
        s->held=v->held;s->kind=v->kind;s->initialized=1;s->last_dir=d;s->repeat_time=0;return;
    }
    s->pressed=(InU16)(v->held&~s->held);s->released=(InU16)(s->held&~v->held);s->held=v->held;
    if(d!=s->last_dir){s->repeat=d;s->last_dir=d;s->repeat_time=0;}
    else if(d && (hz==50 || hz==60)) {
        InU16 before=s->repeat_time;s->repeat_time+=hz==50?6:5;
        if(before<90 && s->repeat_time>=90)s->repeat=d;
        if(s->repeat_time>=120){s->repeat=d;s->repeat_time-=30;}
    }
    if(v->kind==IN_MOUSE) {
        InS16 x=(InS16)(s->x+v->dx),y=(InS16)(s->y+v->dy);
        s->x=x<0?0:(x>312?312:x);s->y=y<184?184:(y>216?216:y);
    }
}
InU16 in_join(InSlot slots[8],InU16 index) {
    if(index>=8 || !slots[index].initialized || !(slots[index].pressed&IN_START))return 0;
    if(slots[index].player)return slots[index].player;
    for(InU16 player=1;player<=8;player++) {
        InU16 taken=0;for(InU16 i=0;i<8;i++)if(slots[i].player==player)taken=1;
        if(!taken){slots[index].player=player;return player;}
    }
    return 0;
}

#include "animation.h"
#define EMPTY {0,0,0,0}
static AnFrame const idle[]={{0,24,0,AN_NONE,EMPTY}};
static AnFrame const walk[]={
 {1,7,AN_FOOT,AN_NONE,EMPTY},{0,7,0,AN_NONE,EMPTY},
 {2,7,AN_FOOT,AN_NONE,EMPTY},{0,7,0,AN_NONE,EMPTY}};
static AnFrame const attack[]={
 {3,5,0,AN_NONE,EMPTY},
 {3,3,AN_SWISH,0,{8,-26,20,14}},
 {3,3,0,1,{8,-18,26,14}},
 {3,3,0,2,{4,-8,24,14}},
 {0,9,0,AN_NONE,EMPTY}};
static AnFrame const hurt[]={{4,6,0,AN_NONE,EMPTY},{4,6,0,AN_NONE,EMPTY},{0,6,0,AN_NONE,EMPTY}};
static AnFrame const death[]={{4,8,0,AN_NONE,EMPTY},{5,10,0,AN_NONE,EMPTY},{6,12,0,AN_NONE,EMPTY}};
static AnFrame const rise[]={{5,12,0,AN_NONE,EMPTY},{0,12,0,AN_NONE,EMPTY}};
AnClip const an_clips[AN_CLIPS]={
 {"IDLE",idle,1,1},{"WALK",walk,4,1},{"ATTACK",attack,5,0},
 {"HURT",hurt,3,0},{"DEATH",death,3,0},{"RISE",rise,2,0}};
int an_validate(AnClip const *c) {
 if(!c || !c->name || !c->frames || !c->count || c->count>8 || c->loop>1)return 0;
 for(AnU16 i=0;i<c->count;i++) {
  AnFrame const *f=&c->frames[i];
  if(!f->hold || f->hold>600 || f->pose>6 || (f->events&~(AN_FOOT|AN_SWISH)) || (f->weapon!=AN_NONE && f->weapon>2))return 0;
  if((!f->hit.w)!=(!f->hit.h) || f->hit.w>32 || f->hit.h>32 || f->hit.x < -64 || f->hit.x>64 || f->hit.y < -64 || f->hit.y>64)return 0;
 }
 return 1;
}
AnFrame const *an_pose(AnPlayer const *p) {
 if(!p || p->clip>=AN_CLIPS)return 0;
 AnClip const *c=&an_clips[p->clip];if(p->frame>=c->count)return 0;
 return &c->frames[p->frame];
}
AnU16 an_play(AnPlayer *p,AnU16 clip,AnU16 restart) {
 if(!p || clip>=AN_CLIPS)return AN_ERROR;
 if(p->generation && p->clip==clip && !restart)return 0;
 if(!an_validate(&an_clips[clip]))return AN_ERROR;
 p->clip=clip;p->frame=0;p->age=0;p->finished=0;
 p->generation=(AnU16)(p->generation+1u);if(!p->generation)p->generation=1;
 return (AnU16)(AN_STARTED|an_clips[clip].frames[0].events);
}
AnU16 an_step(AnPlayer *p) {
 AnFrame const *f=an_pose(p);if(!f || p->age>=f->hold)return AN_ERROR;
 if(p->finished)return 0;
 p->age++;if(p->age<f->hold)return 0;
 AnClip const *c=&an_clips[p->clip];
 if(p->frame+1u<c->count){p->frame++;p->age=0;return c->frames[p->frame].events;}
 if(c->loop){p->frame=0;p->age=0;return c->frames[0].events;}
 p->finished=1;p->age=(AnU16)(f->hold-1u);return AN_COMPLETE;
}
int an_hitbox(AnPlayer const *p,AnU16 left,AnS16 x,AnS16 y,AnBox *out) {
 AnFrame const *f=an_pose(p);if(!f || !out || left>1 || !f->hit.w || !f->hit.h || p->finished)return 0;
 long bx=(long)x+(left?-(long)f->hit.x-(long)f->hit.w:(long)f->hit.x),by=(long)y+f->hit.y;
 if(bx < -32768 || bx>32767 || by < -32768 || by>32767)return 0;
 out->x=(AnS16)bx;out->y=(AnS16)by;out->w=f->hit.w;out->h=f->hit.h;return 1;
}
int an_overlap(AnBox const *a,AnBox const *b) {
 if(!a || !b || !a->w || !a->h || !b->w || !b->h)return 0;
 return (long)a->x < (long)b->x+b->w && (long)b->x < (long)a->x+a->w &&
        (long)a->y < (long)b->y+b->h && (long)b->y < (long)a->y+a->h;
}

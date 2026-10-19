#include "combat.h"
static CoS16 absolute(CoS16 n){return n<0?(CoS16)-n:n;}
static CoU16 face(CoS16 dx,CoS16 dy){return absolute(dx)>absolute(dy)?(dx<0?2:3):(dy<0?1:0);}
CoU16 co_overlap(CoBox const *a,CoBox const *b){
 return a->w>0 && a->h>0 && b->w>0 && b->h>0 && (int)a->x+a->w>b->x && (int)b->x+b->w>a->x && (int)a->y+a->h>b->y && (int)b->y+b->h>a->y;
}
CoU16 co_attack_box(CoActor const *a,CoBox *b){
 if(a->state!=CO_SWING || a->age<6 || a->age>=13)return 0;
 b->x=a->x-17;b->y=a->y-17;b->w=34;b->h=34;
 if(a->face==0){b->y=a->y+1;b->h=32;}
 else if(a->face==1){b->y=a->y-33;b->h=32;}
 else if(a->face==2){b->x=a->x-33;b->w=32;}
 else{b->x=a->x+1;b->w=32;}
 return 1;
}
CoU16 co_front(CoActor const *a,CoS16 x,CoS16 y){
 CoS16 dx=(CoS16)(x-a->x),dy=(CoS16)(y-a->y);
 if(a->face==0)return dy>0 && absolute(dx)<=dy;
 if(a->face==1)return dy<0 && absolute(dx)<=-dy;
 if(a->face==2)return dx<0 && absolute(dy)<=-dx;
 return dx>0 && absolute(dy)<=dx;
}
CoU16 co_blocked(CoS16 x,CoS16 y){
 /* Ten-by-eight foot box, with two decorative arena plinths. */
 if(x<21 || x>298 || y<22 || y>171)return 1;
 return ((x>107 && x<133)||(x>187 && x<213)) && y>75 && y<101;
}
static void move(CoActor *a,CoS16 dx,CoS16 dy){
 if(!co_blocked((CoS16)(a->x+dx),a->y))a->x+=dx;
 if(!co_blocked(a->x,(CoS16)(a->y+dy)))a->y+=dy;
}
void co_swing(CoActor *a){if(a->state==CO_IDLE && a->joined && a->hp){a->state=CO_SWING;a->age=0;a->victims=0;a->serial++;a->guard=0;}}
void co_init(CoWorld *w){
 for(CoU16 i=0;i<sizeof(*w);i++)((volatile CoU8 *)w)[i]=0;
 w->help=1;w->round=1;
 for(CoU16 i=0;i<CO_ACTORS;i++){
  CoActor *a=&w->actor[i];a->generation=1;a->joined=i!=1;a->hp=i<2?5:3;
  a->x=i<2?(CoS16)(48+i*24):(CoS16)(184+(i-2)*44);a->y=i<2?144:(CoS16)(40+(i-2)*36);a->face=i<2?1:0;
 }
}
static CoU16 hostile(CoU16 a,CoU16 b){return (a<2)!=(b<2);}
static void hurt(CoActor *a,CoS16 x,CoS16 y){
 if(a->hp)a->hp--;
 a->state=a->hp?CO_HURT:CO_DEAD;a->age=0;a->guard=0;a->iframes=45;a->flash=12;
 a->kx=a->x>x?2:a->x<x?-2:0;a->ky=a->y>y?2:a->y<y?-2:0;
}
/* Gather all outcomes before changing state: mutual active hits trade.
   First damage source in stable actor/bolt order supplies knockback. */
void co_resolve(CoWorld *w){
 CoU16 damage[CO_ACTORS]={0,0,0,0,0};CoS16 sx[CO_ACTORS]={0,0,0,0,0},sy[CO_ACTORS]={0,0,0,0,0};
 w->candidates=0;
 for(CoU16 i=0;i<CO_ACTORS;i++){
  CoActor *a=&w->actor[i];CoBox attack;
  if(!a->joined || !a->hp || !co_attack_box(a,&attack))continue;
  for(CoU16 j=0;j<CO_ACTORS;j++){
   CoActor *b=&w->actor[j];CoU16 bit=(CoU16)(1u<<j);
   if(!hostile(i,j) || !b->joined || !b->hp || (a->victims&bit))continue;
   w->candidates++;CoBox body={(CoS16)(b->x-6),(CoS16)(b->y-12),12,16};
   if(!co_overlap(&attack,&body))continue;
   a->victims|=bit; /* Also consume blocked/invulnerable contact this swing. */
   if(b->iframes)continue;
   if(b->guard && co_front(b,a->x,a->y)){w->blocks++;b->flash=8;w->events|=CO_BLOCK;continue;}
   if(!damage[j]){damage[j]=1;sx[j]=a->x;sy[j]=a->y;}
  }
 }
 for(CoU16 i=0;i<CO_BOLTS;i++){
  CoBolt *p=&w->bolt[i];if(!p->life)continue;
  if(p->owner>=CO_ACTORS || !w->actor[p->owner].joined || w->actor[p->owner].generation!=p->generation){p->life=0;continue;}
  for(CoU16 j=0;j<CO_ACTORS;j++){
   CoActor *b=&w->actor[j];if(!hostile(p->owner,j) || !b->joined || !b->hp)continue;
   w->candidates++;CoBox shot={(CoS16)(p->x-3),(CoS16)(p->y-3),6,6},body={(CoS16)(b->x-6),(CoS16)(b->y-12),12,16};
   if(!co_overlap(&shot,&body))continue;
   p->life=0;
   if(b->iframes)break;
   /* Incoming velocity, not a possibly far-away owner, determines guard side. */
   if(b->guard && co_front(b,(CoS16)(b->x-p->dx),(CoS16)(b->y-p->dy))){w->events|=CO_BLOCK;b->flash=8;w->blocks++;break;}
   if(!damage[j]){damage[j]=1;sx[j]=(CoS16)(b->x-p->dx);sy[j]=(CoS16)(b->y-p->dy);}break;
  }
 }
 for(CoU16 i=0;i<CO_ACTORS;i++)if(damage[i]){hurt(&w->actor[i],sx[i],sy[i]);w->hits++;w->events|=i<2?CO_DAMAGE:CO_HIT;}
 if(w->candidates>w->work_high)w->work_high=w->candidates;
}
static void fire(CoWorld *w,CoU16 owner){
 CoActor *a=&w->actor[owner];
 for(CoU16 i=0;i<CO_BOLTS;i++)if(!w->bolt[i].life){
  CoBolt *p=&w->bolt[i];p->x=a->x;p->y=a->y-4;p->dx=a->face==2?-4:a->face==3?4:0;p->dy=a->face==1?-4:a->face==0?4:0;
  p->life=70;p->owner=owner;p->generation=a->generation;w->events|=CO_SHOOT;a->cooldown=24;return;
 }
}
static void player(CoWorld *w,CoU16 n,CoU16 held,CoU16 pressed){
 CoActor *a=&w->actor[n];a->moving=0;
 if(!a->joined || !a->hp || a->state!=CO_IDLE)return;
 CoS16 dx=(held&CO_LEFT)?-1:(held&CO_RIGHT)?1:0,dy=(held&CO_UP)?-1:(held&CO_DOWN)?1:0;
 if(dx || dy)a->face=face(dx,dy);
 a->guard=(held&CO_GUARD)!=0;
 if((pressed&CO_ATTACK) && !a->guard){co_swing(a);w->events|=CO_SWISH;return;}
 if((pressed&CO_FIRE) && !a->guard && !a->cooldown)fire(w,n);
 if(a->guard && (w->tick&1u))return;
 move(a,dx,dy);a->moving=dx || dy;
}
void co_tick(CoWorld *w,CoU16 h1,CoU16 h2,CoU16 p1,CoU16 p2){
 w->tick++;
 if(w->complete || w->game_over)return;
 for(CoU16 i=0;i<CO_ACTORS;i++){
  CoActor *a=&w->actor[i];if(a->flash)a->flash--;if(a->iframes)a->iframes--;if(a->cooldown)a->cooldown--;
  if(a->state==CO_SWING){a->age++;if(a->age>=24){a->state=CO_IDLE;a->age=0;}}
  else if(a->state==CO_HURT){a->age++;if(a->age<9)move(a,a->kx,a->ky);if(a->age>=16){a->state=CO_IDLE;a->age=0;}}
 }
 player(w,0,h1,p1);player(w,1,h2,p2);
 for(CoU16 i=2;i<CO_ACTORS;i++){
  CoActor *a=&w->actor[i];if(!a->hp || a->state!=CO_IDLE)continue;
  CoU16 target=0;
  if(w->actor[1].joined && w->actor[1].hp && (!w->actor[0].hp || absolute(a->x-w->actor[1].x)+absolute(a->y-w->actor[1].y)<absolute(a->x-w->actor[0].x)+absolute(a->y-w->actor[0].y)))target=1;
  CoActor const *b=&w->actor[target];CoS16 dx=(CoS16)(b->x-a->x),dy=(CoS16)(b->y-a->y);a->face=face(dx,dy);
  /* Delayed periodic guard windows have recovery gaps; not perfect counters. */
  CoU16 phase=(CoU16)((w->tick+i*17u)&127u);a->guard=i!=2 && phase>=32 && phase<65;
  CoU16 distance=(CoU16)(absolute(dx)+absolute(dy));
  if(!a->guard && distance<40 && !a->cooldown){co_swing(a);a->cooldown=100;}
  else if(i==4 && !a->guard && distance>=40 && !a->cooldown){fire(w,i);a->cooldown=110;}
  else if(!a->guard && distance>29 && (w->tick&1u)==0){move(a,dx<0?-1:dx>0?1:0,dy<0?-1:dy>0?1:0);a->moving=1;}
 }
 for(CoU16 i=0;i<CO_BOLTS;i++){
  CoBolt *p=&w->bolt[i];if(!p->life)continue;
  p->x+=p->dx;p->y+=p->dy;p->life--;
  if(co_blocked(p->x,p->y))p->life=0;
 }
 co_resolve(w);
 if(!w->actor[2].hp && !w->actor[3].hp && !w->actor[4].hp){w->complete=1;w->events|=CO_CLEAR;}
 if(!w->actor[0].hp && (!w->actor[1].joined || !w->actor[1].hp))w->game_over=1;
}
CoU16 co_advance(CoWorld *w,CoU16 elapsed,CoU16 h1,CoU16 h2,CoU16 p1,CoU16 p2){
 w->events=0;
 if(w->help){if(p1&(CO_ATTACK|CO_START)){w->help=0;w->clock=0;}return 0;}
 if(p2&CO_START){CoActor *a=&w->actor[1];if(!a->joined){
  static CoS16 const dx[4]={16,-16,0,0},dy[4]={0,0,16,-16};
  a->joined=1;a->x=48;a->y=144;
  for(CoU16 i=0;i<4;i++){
   CoS16 x=(CoS16)(w->actor[0].x+dx[i]),y=(CoS16)(w->actor[0].y+dy[i]);
   if(!co_blocked(x,y)){a->x=x;a->y=y;break;}
  }
  a->iframes=60;
 }}
 if((w->complete || w->game_over) && (p1&CO_GUARD)){co_init(w);w->help=0;return 0;}
 if(p1&CO_START){w->paused^=1;w->clock=w->pending[0]=w->pending[1]=0;return 0;}
 if(w->paused)return 0;
 w->pending[0]|=p1;w->pending[1]|=p2;
 if(elapsed>20){elapsed=20;w->clamped++;}
 w->clock+=elapsed;CoU16 steps=0;
 while(w->clock>=5){w->clock-=5;co_tick(w,h1,h2,w->pending[0],w->pending[1]);w->pending[0]=w->pending[1]=0;steps++;}
 return steps;
}

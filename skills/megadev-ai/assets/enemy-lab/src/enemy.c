#include "enemy.h"
char const ai_rows[AI_H][AI_W+1]={
 "####################",
 "#......#...........#",
 "#......#...........>",
 "#..##..#..###......>",
 "#......#...........#",
 "#.............##...#",
 "#..##.....#........#",
 "#.........#..###...#",
 "#..###....#........#",
 "#.........#........#",
 "#..................#",
 "####################"
};
/* The freestanding debug link has no implicit libgcc multiply helper.
   Both factors are words; original 68000 MULU returns the full 32-bit result. */
AiU32 ai_product(AiU16 a,AiU16 b){
#if defined(__m68k__)
 AiU32 result=a;
 __asm__("mulu.w %1,%0" : "+d"(result) : "dm"(b) : "cc");
 return result;
#else
 return (AiU32)a*b;
#endif
}
static AiS16 absolute(AiS16 a){return a<0?(AiS16)-a:a;}
static AiU16 cell(AiS16 x,AiS16 y){return x<0 || y<0 || x>=320 || y>=192?AI_NONE:(AiU16)((y>>4)*AI_W+(x>>4));}
static AiU16 solid(AiMap const *m,AiS16 x,AiS16 y){return x<0 || y<0 || x>=AI_W || y>=AI_H?1:m->solid[y*AI_W+x]!=0;}
AiU16 ai_begin(AiSearch *q,AiMap const *m,AiU16 start,AiU16 goal,AiU16 owner,AiU16 generation){
 /* A caller must retire/cancel its current job before reusing this workspace. */
 if(q->status==AI_WORK)return 0;
 q->status=AI_INVALID;q->last_work=q->expanded=q->head=q->tail=0;
 if(start>=AI_CELLS || goal>=AI_CELLS || m->solid[start] || m->solid[goal])return 0;
 for(AiU16 i=0;i<AI_CELLS;i++)q->parent[i]=AI_NONE;
 q->start=start;q->goal=goal;q->owner=owner;q->generation=generation;q->revision=m->revision;
 q->parent[start]=(AiU8)start;q->queue[0]=(AiU8)start;q->tail=1;
 q->status=start==goal?AI_FOUND:AI_WORK;return 1;
}
static void discover(AiSearch *q,AiMap const *m,AiU16 from,AiU16 next){
 if(m->solid[next] || q->parent[next]!=AI_NONE)return;
 /* Every cell enters once, so the fixed queue cannot grow beyond the map. */
 if(q->tail>=AI_CELLS){q->status=AI_INVALID;return;}
 q->parent[next]=(AiU8)from;q->queue[q->tail++]=(AiU8)next;
 if(next==q->goal)q->status=AI_FOUND;
}
AiU16 ai_work(AiSearch *q,AiMap const *m,AiU16 budget){
 q->last_work=0;if(q->status!=AI_WORK)return 0;
 if(m->revision!=q->revision){q->status=AI_CANCELLED;return 0;}
 if(budget>AI_BUDGET)budget=AI_BUDGET;
 while(q->status==AI_WORK && q->head<q->tail && q->last_work<budget){
  AiU16 n=q->queue[q->head++],x=n%AI_W;q->expanded++;q->last_work++;
  /* Stable neighbor order makes equal-length route choices deterministic. */
  if(n>=AI_W)discover(q,m,n,n-AI_W);
  if(x+1u<AI_W)discover(q,m,n,n+1u);
  if(n+AI_W<AI_CELLS)discover(q,m,n,n+AI_W);
  if(x)discover(q,m,n,n-1u);
 }
 if(q->status==AI_WORK && q->head==q->tail)q->status=AI_NO_PATH;
 return q->last_work;
}
AiU16 ai_path(AiSearch const *q,AiU8 *out,AiU16 capacity){
 if(q->status!=AI_FOUND || q->start>=AI_CELLS || q->goal>=AI_CELLS)return 0;
 AiU16 n=q->goal,length=0;
 while(n!=q->start){
  if(n>=AI_CELLS || length>=AI_CELLS || q->parent[n]==AI_NONE)return 0;
  length++;n=q->parent[n];
 }
 if(length>capacity || (length && !out))return 0;
 n=q->goal;for(AiU16 i=length;i;i--){out[i-1]=(AiU8)n;n=q->parent[n];}
 return length;
}
AiU16 ai_result_valid(AiSearch const *q,AiMap const *m,AiU16 owner,AiU16 generation){
 return q->status==AI_FOUND && q->revision==m->revision && q->owner==owner && q->generation==generation;
}
AiU16 ai_ray(AiMap const *m,AiS16 x0,AiS16 y0,AiS16 x1,AiS16 y1){
 /* Grid DDA in native pixels. At a corner, both touching side cells block:
    guards cannot see diagonally through two touching solid walls. */
 if(cell(x0,y0)==AI_NONE || cell(x1,y1)==AI_NONE)return 0;
 AiS16 x=x0>>4,y=y0>>4,tx=x1>>4,ty=y1>>4;
 AiS16 dx=absolute((AiS16)(x1-x0)),dy=absolute((AiS16)(y1-y0));
 AiS16 sx=x1>x0?1:-1,sy=y1>y0?1:-1;
 AiU16 nx=(AiU16)(sx>0?16-(x0&15):x0&15),ny=(AiU16)(sy>0?16-(y0&15):y0&15);
 for(AiU16 i=0;i<AI_W+AI_H;i++){
  if(solid(m,x,y))return 0;
  if(x==tx && y==ty)return 1;
  AiU32 vx=ai_product(nx,(AiU16)dy),vy=ai_product(ny,(AiU16)dx);
  if(!dx){y+=sy;ny+=16;}
  else if(!dy){x+=sx;nx+=16;}
  else if(vx==vy){if(solid(m,x+sx,y) || solid(m,x,y+sy))return 0;x+=sx;y+=sy;nx+=16;ny+=16;}
  else if(vx<vy){x+=sx;nx+=16;}else{y+=sy;ny+=16;}
 }
 return 0;
}
AiU16 ai_sees(AiMap const *m,AiEnemy const *e,AiS16 x,AiS16 y){
 AiS16 dx=x-e->x,dy=y-e->y,forward,lateral;
 if(e->state==AI_STUNNED || absolute(dx)+absolute(dy)>(e->kind==2?144:112))return 0;
 if(e->face==0){forward=dy;lateral=dx;}else if(e->face==1){forward=-dy;lateral=dx;}
 else if(e->face==2){forward=-dx;lateral=dy;}else{forward=dx;lateral=dy;}
 return forward>=0 && absolute(lateral)<=forward && ai_ray(m,e->x,e->y,x,y);
}
AiU16 ai_blocked(AiMap const *m,AiS16 x,AiS16 y){
 if(x<5 || y<4 || x>315 || y>188)return 1;
 return solid(m,(x-5)>>4,(y-4)>>4) || solid(m,(x+4)>>4,(y-4)>>4) ||
        solid(m,(x-5)>>4,(y+3)>>4) || solid(m,(x+4)>>4,(y+3)>>4);
}
void ai_change_state(AiEnemy *e,AiU16 state,AiU16 timer){
 e->state=state;e->timer=timer;e->generation++;e->length=e->cursor=0;e->replan=0;
}
void ai_init(AiWorld *s){
 volatile AiU8 *p=(volatile AiU8 *)s;for(AiU16 i=0;i<sizeof(*s);i++)p[i]=0;
 s->map.revision=1;
 for(AiU16 y=0;y<AI_H;y++)for(AiU16 x=0;x<AI_W;x++)s->map.solid[y*AI_W+x]=ai_rows[y][x]=='#';
 s->x=24;s->y=168;s->face=3;s->hp=3;
 for(AiU16 i=0;i<AI_ENEMIES;i++){
  AiEnemy *e=&s->enemies[i];e->kind=i;e->generation=1;e->face=i==2?2:0;
  e->x=e->home_x=i==0?88:i==1?264:264;e->y=e->home_y=i==0?40:i==1?152:40;
 }
}
static void face(AiEnemy *e,AiS16 x,AiS16 y){
 AiS16 dx=x-e->x,dy=y-e->y;
 if(absolute(dx)>absolute(dy))e->face=dx<0?2:3;else if(dy)e->face=dy<0?1:0;
}
static void move(AiMap const *m,AiS16 *x,AiS16 *y,AiS16 dx,AiS16 dy){
 if(!ai_blocked(m,(AiS16)(*x+dx),*y))*x+=dx;
 if(!ai_blocked(m,*x,(AiS16)(*y+dy)))*y+=dy;
}
static void toward(AiWorld *s,AiEnemy *e,AiS16 x,AiS16 y){
 AiS16 dx=x-e->x,dy=y-e->y;face(e,x,y);
 /* Cardinal motion prevents diagonal speed gain and corner-cutting. */
 AiS16 ox=e->x,oy=e->y;
 if(absolute(dx)>absolute(dy)){
  move(&s->map,&e->x,&e->y,dx>0?1:-1,0);
  if(e->x==ox && dy)move(&s->map,&e->x,&e->y,0,dy>0?1:-1);
 }else if(dy){
  move(&s->map,&e->x,&e->y,0,dy>0?1:-1);
  if(e->y==oy && dx)move(&s->map,&e->x,&e->y,dx>0?1:-1,0);
 }
}
static void fire(AiWorld *s,AiEnemy *e){
 if(e->cooldown || !e->visible || !ai_ray(&s->map,e->x,e->y,s->x,s->y))return;
 face(e,s->x,s->y);e->cooldown=180;
 for(AiU16 i=0;i<AI_BOLTS;i++)if(!s->bolts[i].life){
  AiBolt *b=&s->bolts[i];b->x=e->x;b->y=e->y;b->life=100;
  AiS16 dx=s->x-e->x,dy=s->y-e->y;
  b->dx=absolute(dx)>=absolute(dy)?(dx<0?-2:2):(dx<-12?-1:dx>12?1:0);
  b->dy=absolute(dy)>=absolute(dx)?(dy<0?-2:2):(dy<-12?-1:dy>12?1:0);
  s->events|=AI_EVENT_FIRE;return;
 }
 s->dropped++;
}
static void hurt(AiWorld *s){
 if(s->hurt || s->game_over || s->complete)return;
 s->hp--;s->hurt=90;s->events|=AI_EVENT_HURT;
 if(!s->hp)s->game_over=1;
}
static void path_service(AiWorld *s){
 AiSearch *q=&s->job;
 if(q->status==AI_WORK){
  if(q->owner>=AI_ENEMIES || s->enemies[q->owner].generation!=q->generation){q->status=AI_CANCELLED;s->discarded++;}
  else s->work+=ai_work(q,&s->map,AI_BUDGET);
 }
 if(q->status==AI_FOUND || q->status==AI_NO_PATH || q->status==AI_CANCELLED || q->status==AI_INVALID){
  if(q->owner<AI_ENEMIES){AiEnemy *e=&s->enemies[q->owner];
   if(ai_result_valid(q,&s->map,q->owner,e->generation)){e->length=(AiU8)ai_path(q,e->route,AI_PATH);e->cursor=0;s->completed++;}
   else if(q->status==AI_NO_PATH)s->no_path++;
  }
  q->status=AI_IDLE;
 }
 if(q->status!=AI_IDLE)return;
 for(AiU16 k=0;k<AI_ENEMIES;k++){
  AiU16 i=s->job_cursor;if(++s->job_cursor==AI_ENEMIES)s->job_cursor=0;
  AiEnemy *e=&s->enemies[i];
  if(e->kind==2 || (e->state!=AI_CHASE && e->state!=AI_RETURN) || e->replan || e->cursor<e->length)continue;
  if(ai_ray(&s->map,e->x,e->y,e->target_x,e->target_y))continue;
  e->replan=24;
  if(ai_begin(q,&s->map,cell(e->x,e->y),cell(e->target_x,e->target_y),i,e->generation))break;
 }
}
static void step(AiWorld *s,AiU16 held,AiU16 pressed){
 s->tick++;if(s->complete || s->game_over)return;
 if(s->hurt)s->hurt--;
 if(s->cooldown)s->cooldown--;
 if(s->pulse)s->pulse--;
 if((pressed&AI_STUN_BUTTON) && !s->cooldown){
  s->cooldown=180;s->pulse=18;s->events|=AI_EVENT_STUN;
  for(AiU16 i=0;i<AI_ENEMIES;i++){AiEnemy *e=&s->enemies[i];
   if(absolute(s->x-e->x)+absolute(s->y-e->y)<=56 && ai_ray(&s->map,s->x,s->y,e->x,e->y))ai_change_state(e,AI_STUNNED,180);
  }
 }
 AiS16 dx=(held&12u)==AI_LEFT?-2:(held&12u)==AI_RIGHT?2:0,dy=(held&3u)==AI_UP?-2:(held&3u)==AI_DOWN?2:0;
 if(dx && dy){dx/=2;dy/=2;}
 if(dx)s->face=dx<0?2:3;
 if(dy)s->face=dy<0?1:0;
 AiS16 oldx=s->x,oldy=s->y;move(&s->map,&s->x,&s->y,dx,dy);s->moving=oldx!=s->x || oldy!=s->y;
 AiEnemy *sense=&s->enemies[s->sense_cursor];if(++s->sense_cursor==AI_ENEMIES)s->sense_cursor=0;
 sense->visible=ai_sees(&s->map,sense,s->x,s->y);
 if(sense->visible){
  sense->target_x=s->x;sense->target_y=s->y;sense->memory=180;
  if(sense->state==AI_PATROL || sense->state==AI_SEARCH || sense->state==AI_RETURN){ai_change_state(sense,AI_ALERT,24);s->events|=AI_EVENT_ALERT;face(sense,s->x,s->y);}
 }
 for(AiU16 i=0;i<AI_ENEMIES;i++){
  AiEnemy *e=&s->enemies[i];if(e->cooldown)e->cooldown--;if(e->replan)e->replan--;
  if(e->state==AI_STUNNED){e->visible=0;if(e->timer && !--e->timer){ai_change_state(e,AI_RETURN,0);e->target_x=e->home_x;e->target_y=e->home_y;}continue;}
  if(e->state==AI_ALERT){if(e->timer && !--e->timer)ai_change_state(e,AI_CHASE,0);}
  else if(e->state==AI_CHASE){
   if(!e->visible && e->memory)e->memory--;
   if(!e->memory){ai_change_state(e,AI_SEARCH,90);continue;}
   if(e->kind==2)fire(s,e);
   else if(e->kind==1 || (s->tick&1u)){
    if(e->cursor<e->length){AiU16 n=e->route[e->cursor];AiS16 x=(n%AI_W)*16+8,y=(n/AI_W)*16+8;toward(s,e,x,y);if(e->x==x && e->y==y)e->cursor++;}
    else toward(s,e,e->target_x,e->target_y);
   }
  }else if(e->state==AI_SEARCH){
   if(!(s->tick&31u))e->face=(e->face+1u)&3u;
   if(e->timer && !--e->timer){ai_change_state(e,AI_RETURN,0);e->target_x=e->home_x;e->target_y=e->home_y;}
  }else if(e->state==AI_RETURN){
   if(absolute(e->x-e->home_x)+absolute(e->y-e->home_y)<=1){ai_change_state(e,AI_PATROL,0);}
   else if(e->cursor<e->length){AiU16 n=e->route[e->cursor];AiS16 x=(n%AI_W)*16+8,y=(n/AI_W)*16+8;toward(s,e,x,y);if(e->x==x && e->y==y)e->cursor++;}
   else toward(s,e,e->home_x,e->home_y);
  }else if(e->kind==2){if(!(s->tick%120u))e->face=(e->face+1u)&3u;}
  else if(s->tick&1u){
   AiS16 x=e->kind==1?(e->patrol?232:280):88,y=e->kind==1?152:(e->patrol?40:104);
   toward(s,e,x,y);if(e->x==x && e->y==y)e->patrol^=1u;
  }
  if(absolute(s->x-e->x)<11 && absolute(s->y-e->y)<10)hurt(s);
 }
 path_service(s);
 for(AiU16 i=0;i<AI_BOLTS;i++){AiBolt *b=&s->bolts[i];if(!b->life)continue;b->life--;b->x+=b->dx;b->y+=b->dy;
  if(solid(&s->map,b->x>>4,b->y>>4))b->life=0;
  else if(absolute(s->x-b->x)<7 && absolute(s->y-b->y)<7){hurt(s);b->life=0;}
 }
 static AiS16 const lx[3]={40,168,264},ly[3]={40,88,152};
 for(AiU16 i=0;i<3;i++)if(!(s->lenses&(1u<<i)) && absolute(s->x-lx[i])+absolute(s->y-ly[i])<12){s->lenses|=(AiU16)(1u<<i);s->events|=AI_EVENT_LENS;}
 if(s->lenses==7 && s->x>=302 && s->y>=32 && s->y<64){s->complete=1;s->events|=AI_EVENT_CLEAR;}
}
AiU16 ai_advance(AiWorld *s,AiU16 elapsed,AiU16 held,AiU16 pressed){
 s->events=0;s->work=0;
 if(pressed&AI_RESET){ai_init(s);return 0;}
 if(pressed&AI_VIEW)s->view=s->view==2?0:s->view+1u;
 if(pressed&AI_PAUSE){s->paused^=1u;s->pending=0;s->clock=0;return 0;}
 if(s->paused)return 0;
 s->pending|=pressed&AI_STUN_BUTTON;
 if(elapsed>20){elapsed=20;s->clamped++;}
 s->clock+=elapsed;AiU16 count=0;
 while(s->clock>=5){s->clock-=5;AiU16 hit=s->pending;s->pending=0;step(s,held,hit);count++;}
 if(s->work>s->work_high)s->work_high=s->work;
 return count;
}

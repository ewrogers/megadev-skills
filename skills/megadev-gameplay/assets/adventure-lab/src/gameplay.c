#include "gameplay.h"
static GpS16 distance(GpS16 a,GpS16 b){return a>b?(GpS16)(a-b):(GpS16)(b-a);}
GpU16 gp_solid(GpU16 area,GpS16 x,GpS16 y) {
 if(x<0 || y<0 || x>=32 || y>=16)return 1;
 /* Both edges have an obvious two-tile gate, not an invisible transition strip. */
 if(x==0 || x==31)return y<7 || y>8;
 if(y==0 || y==15)return 1;
 if(!area) {
  if(x>=7 && x<=8 && y>=3 && y<=6)return 1;
  if(x>=13 && x<=16 && y>=10 && y<=11)return 1;
  if(x>=21 && x<=22 && y>=4 && y<=8)return 1;
 } else {
  if(x>=6 && x<=7 && y>=9 && y<=12)return 1;
  if(x>=13 && x<=16 && y>=3 && y<=4)return 1;
  if(x>=23 && x<=24 && y>=7 && y<=11)return 1;
 }
 return 0;
}
GpU16 gp_blocked(GpU16 area,GpS16 x,GpS16 y) {
 /* Foot box is ten by eight pixels; quarter-pixel maxima are inclusive. */
 if(x<20 || y<16 || x>2028 || y>1008)return 1;
 return gp_solid(area,(GpS16)((x-20)>>6),(GpS16)((y-16)>>6)) ||
        gp_solid(area,(GpS16)((x+19)>>6),(GpS16)((y-16)>>6)) ||
        gp_solid(area,(GpS16)((x-20)>>6),(GpS16)((y+15)>>6)) ||
        gp_solid(area,(GpS16)((x+19)>>6),(GpS16)((y+15)>>6));
}
GpS16 gp_item_x(GpU16 area,GpU16 i) {
 static GpU16 const x[2][6]={{80,176,224,304,384,448},{80,144,208,304,352,448}};
 return i<6?(GpS16)(x[area&1u][i]<<2):0;
}
GpS16 gp_item_y(GpU16 area,GpU16 i) {
 static GpU16 const y[2][6]={{128,64,128,192,64,128},{128,80,192,112,208,128}};
 return i<6?(GpS16)(y[area&1u][i]<<2):0;
}
void gp_camera(Gameplay *s) {
 GpS16 x=(GpS16)((s->x>>2)-160),y=(GpS16)((s->y>>2)-96);
 s->camera_x=x<0?0:x>192?192:(GpU16)x;
 s->camera_y=y<0?0:y>64?64:(GpU16)y;
}
static void reset_enemies(Gameplay *s) {
 for(GpU16 i=0;i<GP_ENEMIES;i++) {
  static GpU16 const x[4]={176,272,352,432},y[4]={112,208,160,80};
  s->enemies[i].x=(GpS16)(x[i]<<2);s->enemies[i].y=(GpS16)(y[i]<<2);
  s->enemies[i].direction=i&1u;s->enemies[i].stun=0;
 }
}
void gp_init(Gameplay *s) {
 s->x=128;s->y=512;s->area=0;s->gems[0]=s->gems[1]=0;s->hp=3;
 s->hurt=s->cooldown=s->pulse=s->face=s->moving=s->tick=0;
 s->paused=s->clock=s->pending=s->events=s->dropped=s->rejected=s->visits=s->complete=0;
 for(GpU16 i=0;i<GP_EFFECTS;i++)s->effects[i].age=0;
 reset_enemies(s);gp_camera(s);
}
GpU16 gp_spawn(Gameplay *s,GpS16 x,GpS16 y,GpU16 kind) {
 if(kind<1 || kind>2)return 0;
 for(GpU16 i=0;i<GP_EFFECTS;i++)if(!s->effects[i].age) {
  s->effects[i].x=x;s->effects[i].y=y;s->effects[i].age=24;s->effects[i].kind=kind;return 1;
 }
 s->rejected++;return 0; /* Cosmetic overload drops the newest effect. */
}
void gp_move(Gameplay *s,GpS16 dx,GpS16 dy) {
 /* Bounded one-pixel substeps avoid tunnelling at this Lab's supported speed.
    Larger/fast objects need swept tests; this is not a general physics solver. */
 if(dx>8)dx=8;
 if(dx<-8)dx=-8;
 if(dy>8)dy=8;
 if(dy<-8)dy=-8;
 while(dx) {
  GpS16 d=dx>4?4:dx<-4?-4:dx;
  GpS16 x=(GpS16)(s->x+d);if(gp_blocked(s->area,x,s->y))break;
  s->x=x;dx=(GpS16)(dx-d);
 }
 while(dy) {
  GpS16 d=dy>4?4:dy<-4?-4:dy;
  GpS16 y=(GpS16)(s->y+d);if(gp_blocked(s->area,s->x,y))break;
  s->y=y;dy=(GpS16)(dy-d);
 }
}
static void step(Gameplay *s,GpU16 held,GpU16 hit) {
 s->tick++;s->moving=0;
 if(s->hurt)s->hurt--;
 if(s->cooldown)s->cooldown--;
 if(s->pulse)s->pulse--;
 for(GpU16 i=0;i<GP_EFFECTS;i++)if(s->effects[i].age)s->effects[i].age--;
 if((hit&GP_PULSE) && !s->cooldown) {
  s->cooldown=180;s->pulse=20;s->events|=GP_EVENT_PULSE;
  for(GpU16 i=0;i<GP_ENEMIES;i++)if(distance(s->x,s->enemies[i].x)<80 && distance(s->y,s->enemies[i].y)<80)s->enemies[i].stun=120;
 }
 GpS16 dx=0,dy=0;
 if((held&12u)==GP_LEFT){dx=-6;s->face=2;}
 if((held&12u)==GP_RIGHT){dx=6;s->face=3;}
 if((held&3u)==GP_UP){dy=-6;s->face=1;}
 if((held&3u)==GP_DOWN){dy=6;s->face=0;}
 if(dx && dy){dx=dx>0?4:-4;dy=dy>0?4:-4;}
 GpS16 oldx=s->x,oldy=s->y;gp_move(s,dx,dy);s->moving=oldx!=s->x || oldy!=s->y;
 /* Gates require actual edge travel. Enter the other area just inside its gate. */
 if((s->x>=2000 && dx>0) || (s->x<=48 && dx<0)) {
  s->x=s->x>1024?64:1984;s->area^=1;s->visits++;s->events|=GP_EVENT_AREA;
  for(GpU16 i=0;i<GP_EFFECTS;i++)s->effects[i].age=0;
  reset_enemies(s);s->hurt=60;gp_camera(s);return;
 }
 for(GpU16 i=0;i<GP_ITEMS;i++)if(!(s->gems[s->area]&(1u<<i)) && distance(s->x,gp_item_x(s->area,i))<40 && distance(s->y,gp_item_y(s->area,i))<40) {
  s->gems[s->area]|=(GpU16)(1u<<i);s->events|=GP_EVENT_GEM;gp_spawn(s,s->x,s->y,1);
 }
 if(!s->complete && s->gems[0]==63 && s->gems[1]==63){s->complete=1;s->events|=GP_EVENT_CLEAR;}
 for(GpU16 i=0;i<GP_ENEMIES;i++) {
  GpEnemy *e=&s->enemies[i];
  if(e->stun){e->stun--;continue;}
  GpS16 x=(GpS16)(e->x+(e->direction?3:-3));
  if(distance(x,(GpS16)((i==0?176:i==1?272:i==2?352:432)<<2))>128 || gp_blocked(s->area,x,e->y))e->direction^=1;
  else e->x=x;
  if(!s->hurt && distance(s->x,e->x)<40 && distance(s->y,e->y)<32) {
   s->hp--;s->hurt=90;s->events|=GP_EVENT_HURT;gp_spawn(s,s->x,s->y,2);
   if(!s->hp){s->hp=3;s->x=128;s->y=512;s->events|=GP_EVENT_RESPAWN;}
  }
 }
 gp_camera(s);
}
GpU16 gp_advance(Gameplay *s,GpU16 elapsed,GpU16 held,GpU16 pressed) {
 s->events=0;
 if(pressed&GP_PAUSE){s->paused^=1;s->pending=0;s->clock=0;return 0;}
 if(s->paused)return 0;
 if(pressed&GP_RESET){gp_init(s);return 0;}
 s->pending|=pressed&GP_PULSE;
 if(elapsed>20){s->dropped++;elapsed=20;} /* At most four fixed steps, no backlog. */
 s->clock=(GpU16)(s->clock+elapsed);GpU16 count=0;
 while(s->clock>=5) {
  s->clock-=5;GpU16 hit=s->pending;s->pending=0;
  step(s,held,hit);count++;
  if(s->events&GP_EVENT_AREA){s->clock=0;break;}
 }
 return count;
}

#include "gameplay.h"
#include <assert.h>
#include <stdio.h>
static unsigned short walk(Gameplay *s,unsigned short ticks,unsigned short held) {
 unsigned short n=0;while(ticks--){n=(unsigned short)(n+gp_advance(s,5,held,0));}return n;
}
int main(void) {
 Gameplay a,b;gp_init(&a);gp_init(&b);
 /* Equal elapsed time, not equal display frames. No collisions along this path. */
 for(unsigned short i=0;i<60;i++)gp_advance(&a,5,GP_RIGHT,0);
 for(unsigned short i=0;i<50;i++)gp_advance(&b,6,GP_RIGHT,0);
 assert(a.x==b.x && a.y==b.y && a.tick==60 && b.tick==60 && !b.clock);
 assert(a.cooldown==b.cooldown && a.enemies[0].x==b.enemies[0].x);
 gp_init(&a);assert(gp_advance(&a,65535,GP_RIGHT,GP_PULSE)==4 && a.tick==4 && a.dropped==1);
 assert(a.cooldown==177 && !a.pending); /* Edge applied once across catch-up ticks. */
 gp_init(&a);gp_advance(&a,1,0,GP_PULSE);assert(a.pending==GP_PULSE && !a.pulse);
 gp_advance(&a,4,0,0);assert(a.pulse==20 && !a.pending);
 gp_advance(&a,5,0,GP_PAUSE);unsigned short tick=a.tick,cool=a.cooldown;
 for(unsigned short i=0;i<200;i++)gp_advance(&a,6,GP_RIGHT,GP_PULSE);
 assert(a.tick==tick && a.cooldown==cool && !a.pending);
 gp_advance(&a,5,0,GP_PAUSE);gp_advance(&a,5,0,0);assert(a.tick==tick+1 && a.cooldown==cool-1);
 gp_init(&a);a.x=424;a.y=320;assert(!gp_blocked(a.area,a.x,a.y));
 for(unsigned short i=0;i<30;i++)gp_move(&a,8,0);
 assert(a.x==428); /* Right edge of foot box stops before wall x=112. */
 short x=a.x;gp_move(&a,8,8);assert(a.x==x && a.y==328); /* Slide along blocked X. */
 for(unsigned short area=0;area<2;area++) {
  assert(!gp_blocked(area,128,512));
  /* Flood the walkable tile centers: all gems and both gates are connected. */
  unsigned short queue[512],seen[512]={0},head=0,tail=1;queue[0]=8*32+2;seen[queue[0]]=1;
  while(head<tail) {
   unsigned short cell=queue[head++];short tx=(short)(cell%32),ty=(short)(cell/32);
   short const dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
   for(unsigned short d=0;d<4;d++) {
    short nx=(short)(tx+dx[d]),ny=(short)(ty+dy[d]);
    if(nx<0 || nx>=32 || ny<0 || ny>=16 || gp_blocked(area,(short)(nx*64+32),(short)(ny*64+32)))continue;
    unsigned short next=(unsigned short)(ny*32+nx);
    if(!seen[next]){seen[next]=1;assert(tail<512);queue[tail++]=next;}
   }
  }
  assert(seen[8*32] && seen[8*32+31]);
  for(unsigned short i=0;i<6;i++)assert(seen[(gp_item_y(area,i)>>6)*32+(gp_item_x(area,i)>>6)]);

  for(unsigned short i=0;i<6;i++)assert(!gp_blocked(area,gp_item_x(area,i),gp_item_y(area,i)));
 }
 gp_init(&a);a.x=1996;a.y=512;gp_advance(&a,5,GP_RIGHT,0);
 assert(a.area==1 && a.visits==1 && a.x==64 && (a.events&GP_EVENT_AREA));
 assert(a.camera_x==0 && a.camera_y==32);
 gp_advance(&a,5,GP_RIGHT,0);assert(a.area==1); /* No immediate ping-pong. */
 a.x=49;gp_advance(&a,5,GP_LEFT,0);assert(a.area==0 && a.x==1984 && a.camera_x==192);
 gp_init(&a);for(unsigned short i=0;i<GP_EFFECTS;i++)assert(gp_spawn(&a,128,512,1));
 assert(!gp_spawn(&a,128,512,1) && a.rejected==1);walk(&a,24,0);
 assert(gp_spawn(&a,128,512,2)); /* Expired slot can be reused. */
 gp_init(&a);a.x=a.enemies[0].x;a.y=a.enemies[0].y;gp_advance(&a,5,0,GP_PULSE);
 assert(a.enemies[0].stun && a.hp==3);walk(&a,10,0);assert(a.hp==3);
 a.enemies[0].stun=0;a.x=a.enemies[0].x;a.y=a.enemies[0].y;
 gp_advance(&a,5,0,0);assert(a.hp==2 && a.hurt==90 && (a.events&GP_EVENT_HURT));
 for(unsigned short i=0;i<20;i++){a.x=a.enemies[0].x;a.y=a.enemies[0].y;gp_advance(&a,5,0,0);}
 assert(a.hp==2); /* Damage cannot repeat every contact tick. */
 a.hp=1;a.hurt=0;a.x=a.enemies[0].x;a.y=a.enemies[0].y;gp_advance(&a,5,0,0);
 assert(a.hp==3 && a.x==128 && a.y==512 && (a.events&GP_EVENT_RESPAWN));
 gp_init(&a);
 for(unsigned short area=0;area<2;area++)for(unsigned short i=0;i<6;i++) {
  a.area=area;a.x=gp_item_x(area,i);a.y=gp_item_y(area,i);a.hurt=90;
  gp_advance(&a,5,0,0);assert(a.events&GP_EVENT_GEM);
 }
 assert(a.complete && (a.events&GP_EVENT_CLEAR));gp_advance(&a,5,0,0);assert(!(a.events&GP_EVENT_CLEAR));
 a.tick=65535;gp_advance(&a,5,0,0);assert(!a.tick); /* Defined unsigned wrap. */
 /* Long runs remain bounded and never walk through any solid map cell. */
 gp_init(&a);unsigned short rng=0x1234;
 for(unsigned long i=0;i<20000;i++) {
  rng=(unsigned short)((rng>>1)^((rng&1)?0xB400:0));
  gp_advance(&a,(unsigned short)(i&1?6:5),(unsigned short)(1u<<(rng&3)),(rng&255)==0?GP_PULSE:0);
  assert(!gp_blocked(a.area,a.x,a.y));assert(a.camera_x<=192 && a.camera_y<=64);
  assert(a.hp>=1 && a.hp<=3);
 }
 puts("gameplay: PAL/NTSC clock, bounded catch-up, input edges, sliding, gates, pools, damage and collection passed");return 0;
}

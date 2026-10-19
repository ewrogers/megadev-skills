#include "animation.h"
#include "training.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned short run(Training *s,unsigned short ticks,unsigned short held) {
 unsigned short count=0;while(ticks--)count=(unsigned short)(count+tr_advance(s,5,held,0));return count;
}
int main(void) {
 AnPlayer p={0};AnBox box={1,2,3,4},before=box;
 assert(sizeof(Training)==48 && sizeof(AnPlayer)==10 && sizeof(AnFrame)==16);
 for(unsigned short i=0;i<AN_CLIPS;i++)assert(an_validate(&an_clips[i]));
 AnFrame frame={0,0,0,AN_NONE,{0,0,0,0}};AnClip bad={"BAD",&frame,1,0};
 assert(!an_validate(&bad));frame.hold=1;assert(an_validate(&bad));
 frame.hit.w=1;assert(!an_validate(&bad));frame.hit.h=1;assert(an_validate(&bad));
 frame.events=AN_COMPLETE;assert(!an_validate(&bad));frame.events=0;
 frame.weapon=3;assert(!an_validate(&bad));frame.weapon=AN_NONE;
 bad.count=9;assert(!an_validate(&bad));bad.count=1;bad.loop=2;assert(!an_validate(&bad));
 assert(an_play(&p,AN_WALK,0)==(AN_STARTED|AN_FOOT));
 unsigned short foot=1,generation=p.generation;
 for(unsigned short i=0;i<56;i++) {
  assert(!an_play(&p,AN_WALK,0));unsigned short e=an_step(&p);
  if(e&AN_FOOT)foot++;
  assert(!(e&AN_COMPLETE));
 }
 assert(foot==5 && p.generation==generation && !p.age && !p.frame);
 AnPlayer saved=p;assert(an_play(&p,AN_CLIPS,0)==AN_ERROR && !memcmp(&p,&saved,sizeof(p)));
 assert(an_play(&p,AN_ATTACK,1)&AN_STARTED);
 assert(!an_hitbox(&p,0,200,144,&box) && !memcmp(&box,&before,sizeof(box)));
 for(unsigned short i=0;i<4;i++)assert(!an_step(&p));
 assert(an_step(&p)==AN_SWISH && p.frame==1);
 assert(an_hitbox(&p,0,200,144,&box) && box.x==208 && box.y==118 && box.w==20);
 assert(an_hitbox(&p,1,200,144,&box) && box.x==172 && box.y==118);
 assert(!an_hitbox(&p,2,200,144,&box));
 assert(!an_hitbox(&p,0,32767,144,&box));
 unsigned short done=0;
 for(unsigned short i=0;i<100;i++)if(an_step(&p)&AN_COMPLETE)done++;
 assert(done==1 && p.finished && p.frame==4);
 assert(!an_hitbox(&p,0,200,144,&box));
 p.generation=65535;assert(an_play(&p,AN_IDLE,1)&AN_STARTED);assert(p.generation==1);
 AnBox a={32760,0,20,10},b={32767,0,1,1};assert(an_overlap(&a,&b));
 a.x=0;a.w=10;b.x=10;assert(!an_overlap(&a,&b));b.x=9;assert(an_overlap(&a,&b));
 Training s,t;tr_init(&s);tr_init(&t);
 for(unsigned short i=0;i<60;i++)tr_advance(&s,5,TR_RIGHT,0);
 for(unsigned short i=0;i<50;i++)tr_advance(&t,6,TR_RIGHT,0);
 assert(!memcmp(&s,&t,sizeof(s)) && s.tick==60 && s.steps==5);
 tr_init(&s);s.x=812;tr_advance(&s,5,0,TR_A);run(&s,23,0);
 assert(s.hits==1 && s.contacts>1 && s.swishes==1 && s.completions==1 && s.player.clip==AN_IDLE);
 run(&s,30,TR_A);assert(s.hits==1); /* Held A is not a new action edge. */
 tr_advance(&s,5,0,TR_A);run(&s,5,0);assert(s.hits==2 && s.player.frame==1);
 unsigned short hits=s.hits;tr_advance(&s,5,0,TR_B);
 assert(s.player.clip==AN_HURT && !an_hitbox(&s.player,0,203,144,&box));
 run(&s,20,0);assert(s.hits==hits && s.player.clip==AN_IDLE);
 tr_advance(&s,5,TR_DOWN,TR_B);assert(s.player.clip==AN_DEATH);
 tr_advance(&s,5,0,TR_A);assert(s.player.clip==AN_DEATH);run(&s,29,0);
 assert(s.player.finished && s.player.frame==2);
 saved=s.player;run(&s,30,0);assert(!memcmp(&s.player,&saved,sizeof(saved)));
 tr_advance(&s,5,0,TR_A);assert(s.player.clip==AN_RISE);run(&s,24,0);assert(s.player.clip==AN_IDLE);
 tr_init(&s);s.x=812;tr_advance(&s,5,0,TR_A);run(&s,5,0);
 tr_advance(&s,5,0,TR_START);Training frozen=s;
 for(unsigned short i=0;i<100;i++)tr_advance(&s,6,TR_RIGHT,TR_A);
 assert(!memcmp(&s,&frozen,sizeof(s)));
 tr_advance(&s,5,0,TR_START);run(&s,1,0);assert(!s.paused && s.tick==frozen.tick+1);
 tr_init(&s);assert(tr_advance(&s,65535,0,TR_A)==4 && s.dropped==1 && s.player.clip==AN_ATTACK && s.player.age==3);
 tr_init(&s);tr_advance(&s,1,TR_DOWN,TR_B);assert(s.pending==(TR_B|TR_DOWN));
 tr_advance(&s,4,0,0);assert(s.player.clip==AN_DEATH); /* Modifier belongs to the edge. */
 tr_init(&s);tr_init(&t);tr_advance(&s,0,0,TR_UP);tr_advance(&t,0,0,TR_UP);
 for(unsigned short i=0;i<240;i++)tr_advance(&s,5,TR_RIGHT,0);
 for(unsigned short i=0;i<200;i++)tr_advance(&t,6,TR_RIGHT,0);
 assert(!memcmp(&s,&t,sizeof(s)) && s.tick==60);
 puts("animation: clip bounds, frame holds, events, hit latches, interruption, terminal poses, pause and PAL/NTSC passed");return 0;
}

#include "training.h"
static void consume(Training *s,AnU16 events) {
 s->events|=events&(AN_FOOT|AN_SWISH);
 if(events&AN_FOOT)s->steps++;
 if(events&AN_SWISH)s->swishes++;
 if(events&AN_COMPLETE){s->completions++;s->events|=TR_EVENT_DONE;}
}
static void play(Training *s,AnU16 clip) {consume(s,an_play(&s->player,clip,0));}
void tr_init(Training *s) {
 s->player.generation=0;s->x=560;
 s->left=s->clock=s->slow_clock=s->tick=s->paused=s->slow=s->debug=s->pending=0;
 s->hits=s->contacts=s->hit_latched=s->dummy_flash=s->events=s->steps=s->swishes=s->completions=s->interrupts=s->dropped=0;
 play(s,AN_IDLE);
}
void tr_tick(Training *s,AnU16 held,AnU16 hit) {
 s->tick++;
 if(s->dummy_flash)s->dummy_flash--;
 if((hit&TR_B) && s->player.clip!=AN_DEATH) {
  s->interrupts++;s->events|=TR_EVENT_HURT;
  consume(s,an_play(&s->player,(hit&TR_DOWN)?AN_DEATH:AN_HURT,1));
  return; /* No stale active window/event from the interrupted clip. */
 }
 if(s->player.clip==AN_DEATH) {
  if(s->player.finished && (hit&TR_A)){s->events|=TR_EVENT_REVIVE;play(s,AN_RISE);return;}
  consume(s,an_step(&s->player));return;
 }
 if(s->player.clip==AN_ATTACK || s->player.clip==AN_HURT || s->player.clip==AN_RISE) {
  consume(s,an_step(&s->player));
  if(s->player.clip==AN_ATTACK) {
   AnBox box,dummy={225,124,14,20};
   if(an_hitbox(&s->player,s->left,(AnS16)(s->x>>2),144,&box) && an_overlap(&box,&dummy)) {
    s->contacts++; /* Contacts can span poses; damage is latched once per swing. */
    if(!s->hit_latched){s->hit_latched=1;s->hits++;s->dummy_flash=18;s->events|=TR_EVENT_HIT;}
   }
  }
  if(s->player.finished){play(s,AN_IDLE);}
  return;
 }
 if(hit&TR_A){s->hit_latched=0;play(s,AN_ATTACK);return;}
 AnS16 dx=0;
 if((held&12)==TR_LEFT){dx=-6;s->left=1;}
 if((held&12)==TR_RIGHT){dx=6;s->left=0;}
 if(dx) {
  AnS16 x=(AnS16)(s->x+dx);s->x=x<80?80:x>1200?1200:x;
  play(s,AN_WALK);
 } else play(s,AN_IDLE);
 consume(s,an_step(&s->player));
}
AnU16 tr_advance(Training *s,AnU16 elapsed,AnU16 held,AnU16 pressed) {
 s->events=0;
 if(pressed&TR_START){s->paused^=1;s->clock=s->pending=s->slow_clock=0;return 0;}
 if(pressed&TR_C)s->debug^=1;
 if(pressed&TR_UP){s->slow^=1;s->clock=s->slow_clock=0;}
 if(s->paused)return 0;
 s->pending|=pressed&(TR_A|TR_B);
 if((pressed&TR_B) && (held&TR_DOWN))s->pending|=TR_DOWN;
 if(elapsed>20){elapsed=20;s->dropped++;}
 if(s->slow){s->slow_clock=(AnU16)(s->slow_clock+elapsed);elapsed=s->slow_clock>>2;s->slow_clock&=3;}
 s->clock=(AnU16)(s->clock+elapsed);AnU16 steps=0;
 while(s->clock>=5) {
  s->clock-=5;AnU16 hit=s->pending;s->pending=0;
  tr_tick(s,held,hit);steps++;
 }
 return steps;
}

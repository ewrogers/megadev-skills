#include "enemy.h"
#include <system.h>
/* Lab-only PSG owner. Route events to the game's audio owner when adapting.
   No Z80/FM/PCM driver runs alongside this example. */
#define PSG (*(volatile unsigned char *)0xc00011u)
static AiU16 initialized,previous,note=65535,cue,left,fanfare;
static void tone(AiU16 channel,AiU16 period,AiU16 attenuation){
 PSG=(AiU8)(0x80u|(channel<<5)|(period&15u));PSG=(AiU8)((period>>4)&63u);PSG=(AiU8)(0x90u|(channel<<5)|attenuation);
}
void lab_sound(AiU16 tick,AiU16 events,AiU16 paused){
 if(!initialized){for(AiU16 i=0;i<4;i++)PSG=(AiU8)(0x9fu|(i<<5));initialized=1;}
 if(paused){PSG=0x9f;PSG=0xbf;PSG=0xdf;note=65535;previous=tick;return;}
 AiU16 delta=tick-previous;previous=tick;if(delta>4)delta=4;
 if(events&AI_EVENT_CLEAR){fanfare=150;left=0;note=65535;}
 if(fanfare){
  static AiU16 const tune[15]={428,340,285,214,0,285,214,170,143,0,214,170,143,107,107};
  AiU16 phase=(AiU16)((AiU16)(150u-fanfare)/10u),p=tune[phase];
#if VIDEO == PAL
  p=(AiU16)(p-(p>>7));
#endif
  if(p){tone(0,p,4);tone(1,(AiU16)(p<<1),8);}else{PSG=0x9f;PSG=0xbf;}
  if(phase>=10)tone(2,170,10);else PSG=0xdf;
  fanfare=delta>=fanfare?0:(AiU16)(fanfare-delta);
  if(!fanfare){PSG=0x9f;PSG=0xbf;PSG=0xdf;}
  return;
 }
 AiU16 phase=(tick>>5)&15u;
 if(note!=phase){
  static AiU16 const melody[16]={452,0,338,0,402,0,319,0,452,0,379,0,426,338,319,0};
  AiU16 p=melody[phase];
#if VIDEO == PAL
  p=(AiU16)(p-(p>>7));
#endif
  if(p){tone(0,p,12);tone(1,(AiU16)(p<<1),14);}else{PSG=0x9f;PSG=0xbf;}note=phase;
 }
 if(events&AI_EVENT_CLEAR){cue=1;left=40;}
 else if(events&AI_EVENT_HURT){cue=2;left=18;}
 else if(events&AI_EVENT_ALERT){cue=3;left=16;}
 else if(events&AI_EVENT_STUN){cue=4;left=12;}
 else if(events&AI_EVENT_LENS){cue=1;left=10;}
 else if(events&AI_EVENT_FIRE){cue=5;left=8;}
 if(left){
  AiU16 p=cue==1?(AiU16)(100u+(left<<2)):cue==2?(AiU16)(560u+(left<<3)):cue==3?(left>8?190:142):cue==4?(AiU16)(120u+(left<<4)):(AiU16)(260u-(left<<4));
  tone(2,p,6);left=delta>=left?0:(AiU16)(left-delta);
 }else PSG=0xdf;
}

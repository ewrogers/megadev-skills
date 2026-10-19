#include "combat.h"
#include <system.h>
/* Lab-only PSG owner. Route events to the game's audio owner when adapting.
   No Z80/FM/PCM driver runs alongside this example. */
#define PSG (*(volatile unsigned char *)0xc00011u)
static CoU16 initialized,previous,note=65535,cue,left,fanfare;
static void tone(CoU16 channel,CoU16 period,CoU16 attenuation){
 PSG=(CoU8)(0x80u|(channel<<5)|(period&15u));PSG=(CoU8)((period>>4)&63u);PSG=(CoU8)(0x90u|(channel<<5)|attenuation);
}
void lab_sound(CoU16 tick,CoU16 events,CoU16 paused){
 if(!initialized){for(CoU16 i=0;i<4;i++)PSG=(CoU8)(0x9fu|(i<<5));initialized=1;}
 if(paused){PSG=0x9f;PSG=0xbf;PSG=0xdf;note=65535;previous=tick;return;}
 CoU16 delta=tick-previous;previous=tick;if(delta>4)delta=4;
 if(events&CO_CLEAR){fanfare=150;left=0;note=65535;}
 if(fanfare){
  static CoU16 const tune[15]={428,340,285,214,0,285,214,170,143,0,214,170,143,107,107};
  CoU16 phase=(CoU16)((CoU16)(150u-fanfare)/10u),p=tune[phase];
#if VIDEO == PAL
  p=(CoU16)(p-(p>>7));
#endif
  if(p){tone(0,p,4);tone(1,(CoU16)(p<<1),8);}else{PSG=0x9f;PSG=0xbf;}
  if(phase>=10)tone(2,170,10);else PSG=0xdf;
  fanfare=delta>=fanfare?0:(CoU16)(fanfare-delta);
  if(!fanfare){PSG=0x9f;PSG=0xbf;PSG=0xdf;}
  return;
 }
 CoU16 phase=(tick>>5)&15u;
 if(note!=phase){
  static CoU16 const melody[16]={452,0,338,0,402,0,319,0,452,0,379,0,426,338,319,0};
  CoU16 p=melody[phase];
#if VIDEO == PAL
  p=(CoU16)(p-(p>>7));
#endif
  if(p){tone(0,p,12);tone(1,(CoU16)(p<<1),14);}else{PSG=0x9f;PSG=0xbf;}note=phase;
 }
 if(events&CO_CLEAR){cue=1;left=40;}
 else if(events&CO_DAMAGE){cue=2;left=18;}
 else if(events&CO_SWISH){cue=3;left=16;}
 else if(events&CO_BLOCK){cue=4;left=12;}
 else if(events&CO_HIT){cue=1;left=10;}
 else if(events&CO_SHOOT){cue=5;left=8;}
 if(left){
  CoU16 p=cue==1?(CoU16)(100u+(left<<2)):cue==2?(CoU16)(560u+(left<<3)):cue==3?(left>8?190:142):cue==4?(CoU16)(120u+(left<<4)):(CoU16)(260u-(left<<4));
  tone(2,p,6);left=delta>=left?0:(CoU16)(left-delta);
 }else PSG=0xdf;
}

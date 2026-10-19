#include "training.h"
#include <system.h>
/* Standalone Lab-only PSG owner. Existing games use their current audio owner. */
#define PSG (*(volatile unsigned char *)0xC00011u)
static AnU16 initialized,previous,note=65535,cue,left;
static void tone(AnU16 channel,AnU16 period,AnU16 attenuation) {
 PSG=(unsigned char)(0x80u|(channel<<5)|(period&15u));
 PSG=(unsigned char)((period>>4)&63u);
 PSG=(unsigned char)(0x90u|(channel<<5)|attenuation);
}
void lab_sound(AnU16 tick,AnU16 events,AnU16 paused) {
 if(!initialized){for(AnU16 i=0;i<4;i++)PSG=(unsigned char)(0x9Fu|(i<<5));initialized=1;}
 if(paused){PSG=0x9F;PSG=0xBF;PSG=0xDF;note=65535;previous=tick;return;}
 AnU16 delta=(AnU16)(tick-previous);previous=tick;if(delta>4)delta=4;
 AnU16 phase=(tick>>5)&7u;
 if(note!=phase) {
  static AnU16 const melody[8]={338,284,253,319,338,426,379,319};
  AnU16 period=melody[phase];
#if VIDEO == PAL
  period=(AnU16)(period-(period>>7));
#endif
  tone(0,period,12);tone(1,(AnU16)(period<<1),14);note=phase;
 }
 if(events&TR_EVENT_HIT){cue=1;left=8;}
 else if(events&TR_EVENT_HURT){cue=2;left=12;}
 else if(events&AN_SWISH){cue=3;left=6;}
 else if(events&TR_EVENT_REVIVE){cue=1;left=16;}
 else if(events&AN_FOOT){cue=4;left=3;}
 if(left) {
  AnU16 period=cue==1?(AnU16)(96u+(left<<2)):cue==2?(AnU16)(480u+(left<<3)):cue==3?(AnU16)(160u+(left<<5)):800;
  tone(2,period,cue==4?13:6);left=delta>=left?0:(AnU16)(left-delta);
 } else PSG=0xDF;
}

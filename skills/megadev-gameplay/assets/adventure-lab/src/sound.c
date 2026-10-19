#include "gameplay.h"
#include <system.h>
/* Standalone Lab-only PSG owner. No Z80/FM driver is installed, and no Sub
   audio service writes this port. An existing game must use its audio owner. */
#define PSG (*(volatile unsigned char *)0xC00011u)
static GpU16 initialized,previous,note=65535,cue,left;
static void tone(GpU16 channel,GpU16 period,GpU16 attenuation) {
 PSG=(unsigned char)(0x80u|(channel<<5)|(period&15u));
 PSG=(unsigned char)((period>>4)&63u);
 PSG=(unsigned char)(0x90u|(channel<<5)|attenuation);
}
void lab_sound(GpU16 tick,GpU16 events,GpU16 paused) {
 if(!initialized){for(GpU16 i=0;i<4;i++)PSG=(unsigned char)(0x9Fu|(i<<5));initialized=1;}
 if(paused){PSG=0x9F;PSG=0xBF;PSG=0xDF;note=65535;previous=tick;return;}
 GpU16 delta=(GpU16)(tick-previous);previous=tick;if(delta>4)delta=4;
 GpU16 phase=(tick>>5)&7u;
 if(note!=phase) {
  /* A small original celesta-like square-wave figure, not a driver template. */
  static GpU16 const melody[8]={338,284,253,213,253,284,319,284};
  GpU16 period=melody[phase];
#if VIDEO == PAL
  period=(GpU16)(period-(period>>7));
#endif
  tone(0,period,10);tone(1,(GpU16)(period<<1),13);note=phase;
 }
 if(events&GP_EVENT_CLEAR){cue=1;left=24;}
 else if(events&GP_EVENT_HURT){cue=2;left=12;}
 else if(events&GP_EVENT_GEM){cue=1;left=8;}
 else if(events&GP_EVENT_PULSE){cue=3;left=12;}
 if(left) {
  tone(2,cue==1?(GpU16)(96u+(left<<2)):cue==2?(GpU16)(480u+(left<<3)):(GpU16)(160u+(left<<4)),5);
  left=delta>=left?0:(GpU16)(left-delta);
 } else PSG=0xDF;
}

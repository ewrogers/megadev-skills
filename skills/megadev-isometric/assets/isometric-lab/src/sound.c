#include "iso.h"
#include <system.h>
/* Lab-only PSG owner; adapt events into an existing game's audio service. */
#define PSG (*(volatile unsigned char *)0xc00011u)
static IsoU16 initialized,previous,note=65535,left;
static void tone(IsoU16 channel,IsoU16 period,IsoU16 volume){
 PSG=(unsigned char)(0x80u|(channel<<5)|(period&15u));PSG=(unsigned char)((period>>4)&63u);PSG=(unsigned char)(0x90u|(channel<<5)|volume);
}
void lab_sound(IsoU16 tick,IsoU16 event,IsoU16 paused){
 if(!initialized){for(IsoU16 i=0;i<4;i++)PSG=(unsigned char)(0x9fu|(i<<5));initialized=1;}
 if(paused){PSG=0x9f;PSG=0xbf;PSG=0xdf;note=65535;previous=tick;return;}
 IsoU16 delta=(IsoU16)(tick-previous);previous=tick;if(delta>4)delta=4;
 IsoU16 phase=(tick>>4)&31u;
 if(note!=phase){
  static IsoU16 const song[32]={428,0,340,285,0,340,214,0,255,0,285,340,0,428,340,0,382,0,303,255,0,303,191,0,214,255,303,0,340,0,428,0};
  IsoU16 p=song[phase];
#if VIDEO == PAL
  p=(IsoU16)(p-(p>>7));
#endif
  if(p){tone(0,p,11);tone(1,(IsoU16)(p<<1),14);}else{PSG=0x9f;PSG=0xbf;}note=phase;
 }
 if(event)left=12;
 if(left){tone(2,(IsoU16)(142u+(left<<3)),7);left=delta>=left?0:(IsoU16)(left-delta);}else PSG=0xdf;
}

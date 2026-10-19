#include "fx.h"
/* This Lab owns PSG directly. Integrate effects into the game's audio owner. */
#define PSG (*(volatile unsigned char *)0xC00011u)
static FxU initialized,previous,left,last_note=65535;
void lab_sound(FxU tick,FxU events,FxU paused){
 if(!initialized){for(FxU i=0;i<4;i++)PSG=(unsigned char)(0x9Fu|(i<<5));initialized=1;}
 FxU delta=(FxU)(tick-previous);previous=tick;if(delta>4)delta=4;
 if(paused){PSG=0x9F;PSG=0xFF;last_note=65535;return;}
 FxU note=tick>>5;
 if(note!=last_note){static const FxU tones[8]={428,360,320,285,320,360,480,428};FxU n=tones[note&7u];PSG=(unsigned char)(0x80|(n&15));PSG=(unsigned char)(n>>4);PSG=0x9B;last_note=note;}
 if(events)left=12;
 if(left){PSG=0xE6;PSG=(unsigned char)(0xF0u|(15u-(left>>1)));left=delta>=left?0:(FxU)(left-delta);}else PSG=0xFF;
}

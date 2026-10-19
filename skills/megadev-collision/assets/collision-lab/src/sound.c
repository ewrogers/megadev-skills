#include "demo.h"
/* Standalone finite PSG cue owner; no other Lab driver writes the port. */
#define PSG (*(volatile unsigned char *)0xC00011u)
static CcU16 initialized,previous,left;
void lab_sound(CcU16 tick,CcU16 events,CcU16 paused) {
 if(!initialized){for(CcU16 i=0;i<4;i++)PSG=(unsigned char)(0x9Fu|(i<<5));initialized=1;}
 if(paused){PSG=0xDF;previous=tick;return;}
 CcU16 delta=(CcU16)(tick-previous);previous=tick;if(delta>4)delta=4;
 if(events&CD_EVENT_HIT)left=6;
 else if(events&CD_EVENT_FIRE)left=3;
 if(left){CcU16 tone=(CcU16)(160u+(left<<4));PSG=(unsigned char)(0xC0u|(tone&15u));PSG=(unsigned char)(tone>>4);PSG=0xD9;left=delta>=left?0:(CcU16)(left-delta);}
 else PSG=0xDF;
}

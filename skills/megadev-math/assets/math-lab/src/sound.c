#include "motion.h"
/* Standalone finite PSG cue owner; no other Lab driver writes the port. */
#define PSG (*(volatile unsigned char *)0xC00011u)
static MmU16 initialized,previous,left;
void lab_sound(MmU16 tick,MmU16 events,MmU16 paused) {
 if(!initialized){for(MmU16 i=0;i<4;i++)PSG=(unsigned char)(0x9Fu|(i<<5));initialized=1;}
 if(paused){PSG=0xDF;previous=tick;return;}
 MmU16 delta=(MmU16)(tick-previous);previous=tick;if(delta>4)delta=4;
 if(events&1)left=6;
 else if(events&2)left=3;
 if(left){MmU16 tone=(MmU16)(160u+(left<<4));PSG=(unsigned char)(0xC0u|(tone&15u));PSG=(unsigned char)(tone>>4);PSG=0xD9;left=delta>=left?0:(MmU16)(left-delta);}
 else PSG=0xDF;
}

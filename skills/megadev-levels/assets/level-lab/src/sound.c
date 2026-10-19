#include "levels.h"
/* Standalone finite PSG cue owner; no other Lab driver writes the port. */
#define PSG (*(volatile unsigned char *)0xC00011u)
static LvU16 initialized,previous,left;
void lab_sound(LvU16 tick,LvU16 events,LvU16 paused) {
 if(!initialized){for(LvU16 i=0;i<4;i++)PSG=(unsigned char)(0x9Fu|(i<<5));initialized=1;}
 if(paused){PSG=0xDF;previous=tick;return;}
 LvU16 delta=(LvU16)(tick-previous);previous=tick;if(delta>4)delta=4;
 if(events&LV_EVENT_TRAVEL)left=6;
 else if(events&LV_EVENT_WARP)left=3;
 if(left){LvU16 tone=(LvU16)(160u+(left<<4));PSG=(unsigned char)(0xC0u|(tone&15u));PSG=(unsigned char)(tone>>4);PSG=0xD9;left=delta>=left?0:(LvU16)(left-delta);}
 else PSG=0xDF;
}

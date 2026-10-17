#include "probe.h"
unsigned short hw_pattern(unsigned short index, unsigned short seed) {
    return (unsigned short)(((unsigned int)index * 257u) ^ seed);
}
void hw_fill(volatile unsigned short *memory, unsigned short words, unsigned short seed) {
    for(unsigned short i=0;i<words;i++)memory[i]=hw_pattern(i,seed);
}
unsigned short hw_verify(volatile unsigned short const *memory, unsigned short words, unsigned short seed) {
    if(!words)return 0;
    for(unsigned short i=0;i<words;i++)if(memory[i]!=hw_pattern(i,seed))return 0;
    return 1;
}
unsigned short hw_exercise(volatile unsigned short *memory, unsigned short words) {
    static unsigned short const seeds[]={0,0xFFFF,0xAAAA,0x5555};
    for(unsigned short i=0;i<4;i++) {
        hw_fill(memory,words,seeds[i]);
        if(!hw_verify(memory,words,seeds[i]))return 0;
    }
    return 1;
}

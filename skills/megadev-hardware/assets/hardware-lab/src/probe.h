#ifndef HARDWARE_PROBE_H
#define HARDWARE_PROBE_H
/* Portable bounded primitives; callers supply an owned, aligned region. */
unsigned short hw_pattern(unsigned short index, unsigned short seed);
void hw_fill(volatile unsigned short *memory, unsigned short words, unsigned short seed);
unsigned short hw_verify(volatile unsigned short const *memory, unsigned short words, unsigned short seed);
unsigned short hw_exercise(volatile unsigned short *memory, unsigned short words);
#endif

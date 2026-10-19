#ifndef COMPRESSION_LAB_H
#define COMPRESSION_LAB_H
#include "stream.h"
/* Application validation/measurement helpers; not SDK compression APIs. */
int cp_kos_valid(volatile unsigned char const *src,S32 bytes,S32 wanted);
void cp_kos_decode(volatile unsigned char const *src,volatile unsigned char *dst);
void cp_raw_copy(volatile unsigned char const *src,volatile unsigned char *dst,S32 bytes);
S16 cp_elapsed(S16 before,S16 after,S16 blanks,S16 hz);
#define CP_OUTPUT 0x10000u
#endif

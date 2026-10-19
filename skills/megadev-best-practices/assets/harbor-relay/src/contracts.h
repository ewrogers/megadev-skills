#ifndef BP_CONTRACTS_H
#define BP_CONTRACTS_H
typedef unsigned char Bp8;
typedef unsigned short Bp16;
/* Small executable examples, not SDK APIs or an allocation framework. */
int bp_slice(Bp16 capacity,Bp16 offset,Bp16 length);
int bp_read16(Bp8 const *bytes,Bp16 size,Bp16 offset,Bp16 *out);
int bp_elapsed(Bp16 now,Bp16 previous,Bp16 hz,Bp16 *out);
int bp_checkpoint(Bp16 level,Bp16 position,Bp16 flags);
#endif

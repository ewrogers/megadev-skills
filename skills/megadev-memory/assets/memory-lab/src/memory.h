#ifndef MEMORY_MODEL_H
#define MEMORY_MODEL_H
typedef unsigned short M16;
typedef unsigned int M32;
_Static_assert(sizeof(M16)==2 && sizeof(M32)==4,"Expected 16/32-bit memory fields");
typedef struct {M32 capacity,used,peak,generation;M16 pool,pins;} MArena;
typedef struct {M32 offset,size,generation;M16 pool;} MHandle;
typedef struct {M32 generation;M16 pool,active;} MLease;
void ma_init(MArena *a,M32 capacity,M16 pool);
int ma_take(MArena *a,M32 bytes,M16 alignment,MHandle *out);
int ma_valid(MArena const *a,MHandle const *h);
int ma_pin(MArena *a,MHandle const *h,MLease *lease);
int ma_release(MArena *a,MLease *lease);
int ma_reset(MArena *a);
typedef struct {M32 generation;M16 pending,pins;} MView;
int mv_begin(MView *v);
void mv_complete(MView *v);
int mv_pin(MView *v,MLease *lease);
int mv_release(MView *v,MLease *lease);
#define M_WORDS 256
#define M_BUFFER_WORDS 261
#define M_VIEW_POOL 0x574D
M16 memory_value(M16 seed,M16 index);
void memory_fill(volatile M16 *b,M16 seed,M16 token,M16 begin,M16 count);
int memory_verify(volatile M16 const *b,M16 seed,M16 token);
#endif

#include "memory.h"
void ma_init(MArena *a,M32 capacity,M16 pool) {
    a->capacity=capacity;a->used=0;a->peak=0;a->generation=1;a->pool=pool;a->pins=0;
}
int ma_take(MArena *a,M32 bytes,M16 alignment,MHandle *out) {
    if(!bytes || !alignment || alignment>256 || (alignment&(alignment-1u)))return 0;
    M32 mask=(M32)alignment-1;
    if(a->used>0xFFFFFFFFu-mask)return 0;
    M32 offset=(a->used+mask)&~mask;
    if(offset>a->capacity || bytes>a->capacity-offset)return 0;
    out->offset=offset;out->size=bytes;out->generation=a->generation;out->pool=a->pool;
    a->used=offset+bytes;if(a->used>a->peak)a->peak=a->used;
    return 1;
}
int ma_valid(MArena const *a,MHandle const *h) {
    return h->pool==a->pool && h->generation==a->generation && h->size && h->offset<=a->used && h->size<=a->used-h->offset;
}
int ma_pin(MArena *a,MHandle const *h,MLease *lease) {
    if(lease->active || a->pins==65535 || !ma_valid(a,h))return 0;
    lease->pool=a->pool;lease->generation=a->generation;lease->active=1;a->pins++;return 1;
}
int ma_release(MArena *a,MLease *lease) {
    if(!lease->active || lease->pool!=a->pool || lease->generation!=a->generation || !a->pins)return 0;
    lease->active=0;a->pins--;return 1;
}
int ma_reset(MArena *a) {
    if(a->pins || a->generation==0xFFFFFFFFu)return 0;
    a->used=0;a->generation++;return 1;
}
int mv_begin(MView *v) {
    if(v->pending || v->pins || v->generation==0xFFFFFFFFu)return 0;
    v->generation++;v->pending=1;return 1;
}
void mv_complete(MView *v){v->pending=0;}
int mv_pin(MView *v,MLease *lease) {
    if(v->pending || lease->active || v->pins==65535)return 0;
    lease->generation=v->generation;lease->pool=M_VIEW_POOL;lease->active=1;v->pins++;return 1;
}
int mv_release(MView *v,MLease *lease) {
    if(!lease->active || lease->pool!=M_VIEW_POOL || lease->generation!=v->generation || !v->pins)return 0;
    lease->active=0;v->pins--;return 1;
}
M16 memory_value(M16 seed,M16 index){return (M16)(seed^(M16)(index*109u));}
void memory_fill(volatile M16 *b,M16 seed,M16 token,M16 begin,M16 count) {
    if(!count || begin>=M_WORDS || count>M_WORDS-begin)return;
    if(!begin){b[0]=0x4D45;b[1]=seed;b[2]=token;b[3]=M_WORDS;b[260]=0xCAFE;}
    for(M16 i=begin;i<begin+count;i++)b[4+i]=memory_value(seed,i);
}
int memory_verify(volatile M16 const *b,M16 seed,M16 token) {
    if(b[0]!=0x4D45 || b[1]!=seed || b[2]!=token || b[3]!=M_WORDS || b[260]!=0xCAFE)return 0;
    for(M16 i=0;i<M_WORDS;i++)if(b[4+i]!=memory_value(seed,i))return 0;
    return 1;
}

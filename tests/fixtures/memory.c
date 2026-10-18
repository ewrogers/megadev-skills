#include "memory.h"
#include <assert.h>
#include <string.h>
int main(void) {
    MArena a;MHandle h={0},other={0};MLease lease={0};
    for(M16 alignment=1;alignment<=256;alignment=(M16)(alignment*2u)) {
        ma_init(&a,4096,1);
        for(M16 i=0;i<16;i++) {
            assert(ma_take(&a,3,alignment,&h));assert(!(h.offset&(alignment-1u)));
            assert(ma_valid(&a,&h));
        }
    }
    for(M32 capacity=1;capacity<=131072;capacity*=2) {
        ma_init(&a,capacity,1);assert(ma_take(&a,capacity,1,&h));
        MArena before=a;MHandle unchanged=h;
        assert(!ma_take(&a,1,1,&h));assert(!memcmp(&a,&before,sizeof a));assert(!memcmp(&h,&unchanged,sizeof h));
        assert(ma_pin(&a,&h,&lease));assert(!ma_pin(&a,&h,&lease));assert(!ma_reset(&a));
        assert(ma_release(&a,&lease));assert(!ma_release(&a,&lease));
        assert(ma_reset(&a));assert(!ma_valid(&a,&h));assert(a.peak==capacity);
        assert(ma_take(&a,capacity,1,&other));assert(!ma_valid(&a,&h));
    }
    ma_init(&a,4096,1);
    for(M16 align=0;align<260;align++)if(!align || (align&(align-1u)) || align>256)assert(!ma_take(&a,1,align,&h));
    assert(!ma_take(&a,0,2,&h));assert(!ma_take(&a,0xffffffffu,2,&h));
    assert(ma_take(&a,13,2,&h));other=h;other.pool=2;assert(!ma_valid(&a,&other));
    other=h;other.offset=0xffffffffu;assert(!ma_valid(&a,&other));
    other=h;other.size=0xffffffffu;assert(!ma_valid(&a,&other));
    a.used=0xffffffffu;assert(!ma_take(&a,1,256,&h));
    a.used=0;a.generation=0xffffffffu;assert(!ma_reset(&a));
    MView view={0};MLease reader={0};
    assert(mv_pin(&view,&reader));assert(!mv_begin(&view));assert(mv_release(&view,&reader));
    M32 old=view.generation;assert(mv_begin(&view));assert(view.pending && view.generation!=old);
    assert(!mv_pin(&view,&reader));assert(!mv_begin(&view));mv_complete(&view);
    assert(mv_pin(&view,&reader));MLease wrong=reader;wrong.pool=1;assert(!mv_release(&view,&wrong));
    assert(mv_release(&view,&reader));assert(!mv_release(&view,&reader));
    view.generation=0xffffffffu;assert(!mv_begin(&view));
    M16 memory[M_BUFFER_WORDS+2],before[M_BUFFER_WORDS+2];
    for(M32 seed=0;seed<65536;seed+=257) {
        for(M16 i=0;i<M_BUFFER_WORDS+2;i++)memory[i]=0x5678;
        for(M16 start=0;start<M_WORDS;start+=16)memory_fill(memory+1,(M16)seed,5,start,16);
        assert(memory[0]==0x5678 && memory[M_BUFFER_WORDS+1]==0x5678);
        assert(memory_verify(memory+1,(M16)seed,5));assert(!memory_verify(memory+1,(M16)seed,6));
        memory[10]^=1;assert(!memory_verify(memory+1,(M16)seed,5));
    }
    memcpy(before,memory,sizeof memory);
    memory_fill(memory+1,0,0,0,0);memory_fill(memory+1,0,0,250,16);memory_fill(memory+1,0,0,65535,16);
    assert(!memcmp(before,memory,sizeof memory));
    return 0;
}

#include "coord.h"
#include <assert.h>
#include <string.h>
static SyU16 storage[SY_BUFFER_WORDS+2];
static SyU16 *b=storage+1;
static void prepare(SyU16 token,SyU16 seed) {
    storage[0]=0x1234;storage[SY_BUFFER_WORDS+1]=0x5678;
    sy_prepare(b,token,seed);
}
static void guards(void){assert(storage[0]==0x1234 && storage[SY_BUFFER_WORDS+1]==0x5678 && b[SY_GUARD]==0xCAFE);}
int main(void) {
    /* Whole jobs across wrap-prone seed values; exactly one bounded chunk per step. */
    for(unsigned int seed=0;seed<65536;seed+=257) {
        SyWorker w={0};prepare(2,(SyU16)seed);sy_worker_begin(&w,2,SY_BUILD,(SyU16)seed,SY_WORDS);
        for(unsigned int i=0;i<16;i++) {
            sy_worker_step(&w,b,1,0);assert(w.done==(i+1)*SY_CHUNK);guards();
            for(unsigned int j=w.done;j<SY_WORDS;j++)assert(b[SY_HEADER+j]==sy_input((SyU16)seed,(SyU16)j));
        }
        assert(w.phase==SY_RETIRE && w.result==SY_OK && sy_verify(b,2,(SyU16)seed,w.sum));
        assert(!sy_verify(b,3,(SyU16)seed,w.sum));assert(!sy_verify(b,2,(SyU16)seed,(SyU16)(w.sum+1)));
        b[SY_GUARD]^=1;assert(!sy_verify(b,2,(SyU16)seed,w.sum));
    }
    /* Cancellation at every chunk boundary, and invalid-owner checks before dereference. */
    for(unsigned int chunk=0;chunk<16;chunk++) {
        SyWorker w={0};prepare(4,7);sy_worker_begin(&w,4,SY_BUILD,7,SY_WORDS);
        for(unsigned int i=0;i<chunk;i++)sy_worker_step(&w,b,1,0);
        SyU16 before[SY_BUFFER_WORDS];memcpy(before,b,sizeof before);
        sy_worker_step(&w,b,1,1);assert(w.result==SY_CANCELLED && w.done==chunk*16);
        assert(!memcmp(before,b,sizeof before));guards();
        sy_worker_begin(&w,4,SY_BUILD,7,SY_WORDS);sy_worker_step(&w,0,0,0);assert(w.result==SY_BAD_OWNER);
    }
    for(unsigned int count=0;count<260;count++)if(count!=SY_WORDS) {
        SyWorker w={0};sy_worker_begin(&w,2,SY_BUILD,1,(SyU16)count);
        sy_worker_step(&w,0,1,0);assert(w.result==SY_BAD_REQUEST && w.phase==SY_RETIRE);
    }
    for(unsigned int field=0;field<5;field++) {
        SyWorker w={0};prepare(4,7);sy_worker_begin(&w,4,SY_BUILD,7,SY_WORDS);
        b[field==4?SY_GUARD:field]^=1;sy_worker_step(&w,b,1,0);assert(w.result==SY_BAD_DATA && !w.done);
    }
    SyWorker w={0};prepare(4,7);b[SY_HEADER+22]^=1;sy_worker_begin(&w,4,SY_BUILD,7,SY_WORDS);
    sy_worker_step(&w,b,1,0);sy_worker_step(&w,b,1,0);assert(w.result==SY_BAD_DATA && w.done==22);guards();
    sy_worker_begin(&w,2,77,0,SY_WORDS);sy_worker_step(&w,0,1,0);assert(w.result==SY_BAD_REQUEST);
    /* Late/stale replies, missing ownership, explicit retirement and no premature reuse. */
    for(unsigned int hz=50;hz<=60;hz+=10) {
        SyClient c={0};assert(!sy_begin(&c,1,300,1,1));assert(!sy_begin(&c,2,0,1,1));
        assert(!sy_begin(&c,2,300,0,1));assert(!sy_begin(&c,2,300,1,0));
        assert(sy_begin(&c,2,300,1,1));assert(!sy_begin(&c,3,300,1,1));
        assert(sy_poll(&c,0,0,2,0)==1);
        for(unsigned int frame=0;frame<hz;frame++) {
            assert(!sy_poll(&c,99,SY_OK,1,(SyU16)(300/hz)));
            assert(c.phase==(frame==hz-1?SY_DRAIN:SY_WAIT));
        }
        assert(c.discard && c.reason==SY_TIMEOUT);
        assert(!sy_begin(&c,3,300,1,1));assert(!sy_poll(&c,2,SY_OK,2,5));
        assert(sy_poll(&c,2,SY_CANCELLED,1,5)==2 && c.phase==SY_RETIRE);
        assert(!sy_poll(&c,2,SY_CANCELLED,1,5));assert(!sy_begin(&c,3,300,1,1));
        assert(!sy_poll(&c,0,0,5,5));assert(sy_poll(&c,0,0,1,5)==3);
        assert(sy_begin(&c,65535,300,1,1));assert(sy_poll(&c,0,0,2,5)==1);
        sy_cancel(&c);assert(c.phase==SY_DRAIN && c.reason==SY_CANCELLED);
        assert(sy_poll(&c,65535,SY_OK,1,5)==2 && c.discard);
        assert(sy_poll(&c,0,0,1,5)==3);
        assert(sy_begin(&c,2,5,1,1));assert(!sy_poll(&c,0,0,1,5));
        assert(c.phase==SY_STUCK);assert(!sy_begin(&c,3,5,1,1));
    }
    SyClient late={0};assert(sy_begin(&late,2,300,1,1));assert(sy_poll(&late,0,0,2,0)==1);
    assert(sy_poll(&late,2,SY_OK,1,600)==2);
    assert(late.discard && late.reason==SY_TIMEOUT);
    return 0;
}

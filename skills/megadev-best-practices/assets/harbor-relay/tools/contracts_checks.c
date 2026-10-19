#include <assert.h>
#include "contracts.h"
int main(void) {
    Bp8 bytes[]={0x80,0xfe,0x12,0x34};Bp16 out=77;
    assert(bp_read16(bytes,4,1,&out) && out==0xfe12);
    assert(!bp_read16(bytes,4,3,&out) && out==0xfe12);
    assert(!bp_read16(bytes,4,65535,&out) && out==0xfe12);
    assert(!bp_read16(0,4,0,&out));assert(!bp_read16(bytes,4,0,0));
    for(unsigned cap=0;cap<256;cap++)for(unsigned off=0;off<300;off++)
        for(unsigned len=0;len<8;len++)assert(bp_slice(cap,off,len)==(off+len<=cap));
    assert(!bp_slice(65535,65530,9));assert(bp_slice(65535,65530,5));
    assert(bp_elapsed(3,65533,60,&out) && out==30);
    for(unsigned hz=50;hz<=60;hz+=10) {
        unsigned total=0;
        for(unsigned n=1;n<=hz*4;n++){assert(bp_elapsed(n,n-1,hz,&out));total+=out;}
        assert(total==1200);
        assert(bp_elapsed(65535,0,hz,&out) && out==9000);
    }
    out=42;assert(!bp_elapsed(5,4,30,&out) && out==42);
    assert(bp_checkpoint(1,16,0) && bp_checkpoint(2,280,3));
    assert(!bp_checkpoint(3,16,0) && !bp_checkpoint(1,15,0) && !bp_checkpoint(1,281,0) && !bp_checkpoint(1,20,4));
    return 0;
}

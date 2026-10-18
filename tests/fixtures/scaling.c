#include "scale.h"
#include <assert.h>
#include <string.h>
#include <stdint.h>
int main(void) {
    struct {ScU16 before[4];ScLine data[128];ScU16 after[4];} b;
    memset(&b,0xA5,sizeof b);ScRequest r={0,0,2,0};
    ScLine original[128];memcpy(original,b.data,sizeof original);
    assert(!scale_trace(b.data,127,&r));assert(!memcmp(original,b.data,sizeof original));
    r.mode=3;assert(!scale_trace(b.data,128,&r));r.mode=0;
    r.zoom=5;assert(!scale_trace(b.data,128,&r));r.zoom=2;
    r.angle=256;assert(!scale_trace(b.data,128,&r));r.angle=0;
    assert(!scale_trace(0,128,&r) && !scale_trace(b.data,128,0));
    assert(scale_trace(b.data,128,&r));
    for(int y=0;y<128;y++) {
        assert(b.data[y].x==64*8 && b.data[y].y==(64+y)*8);
        assert(b.data[y].dx==2048 && b.data[y].dy==0);
    }
    memcpy(original,b.data,sizeof original);r.mode=1;
    assert(scale_trace(b.data,128,&r));assert(!memcmp(original,b.data,sizeof original));
    r.angle=64;assert(scale_trace(b.data,128,&r));
    assert(b.data[0].x==192*8 && b.data[0].y==64*8);
    assert(b.data[64].dx==0 && b.data[64].dy==2048);
    for(ScU16 mode=0;mode<3;mode++)for(ScU16 angle=0;angle<256;angle++)for(ScU16 z=0;z<5;z++) {
        r=(ScRequest){mode,angle,z,65535};assert(scale_trace(b.data,128,&r));
        for(int y=0;y<128;y++) {
            ScS16 dx=(ScS16)b.data[y].dx,dy=(ScS16)b.data[y].dy;
            assert(dx>=-6144 && dx<=6144 && dy>=-6144 && dy<=6144);
        }
    }
    r=(ScRequest){2,0,2,7};assert(scale_trace(b.data,128,&r));
    for(int y=1;y<128;y++)assert(b.data[y].dx<=b.data[y-1].dx);
    for(int i=0;i<4;i++)assert(b.before[i]==0xA5A5 && b.after[i]==0xA5A5);
    assert(SC_MAP%512==0 && SC_TRACE%8==0 && SC_IMAGE%32==0);
    assert(SC_IMAGE+SC_WORDS*2<=262144);
    return 0;
}

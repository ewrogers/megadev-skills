#include "probe.h"
#include <assert.h>
int main(void) {
    volatile unsigned short region[130];
    region[0]=0xDEAD;region[129]=0xBEEF;
    assert(hw_pattern(1,0x1234)==0x1335);
    assert(hw_pattern(511,0)==0x00FF);
    hw_fill(region+1,128,0x1234);
    assert(region[1]==0x1234 && region[2]==0x1335);
    assert(hw_verify(region+1,128,0x1234));
    region[70]^=1;
    assert(!hw_verify(region+1,128,0x1234));
    assert(hw_exercise(region+1,128));
    assert(hw_verify(region+1,128,0x5555));
    assert(!hw_exercise(region+1,0));
    assert(region[0]==0xDEAD && region[129]==0xBEEF);
    return 0;
}

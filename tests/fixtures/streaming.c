#include "stream.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char data[STREAM_CAP+2048];
static int valid(S32 length,S16 budget) {
    StreamImage v;int result=stream_image_begin(&v,data,length);if(!result)return 0;
    S32 previous=v.offset;unsigned count=0;
    do {
        result=stream_image_step(&v,data,budget);
        assert(v.offset-previous<=128 && v.offset-previous<=budget);previous=v.offset;
        assert(++count<STREAM_CAP+1);
    }while(!result);
    return result==1;
}
int main(int argc,char **argv) {
    assert(argc==3);
    assert(!stream_rounded(0,STREAM_CAP));assert(stream_rounded(1,STREAM_CAP)==2048);
    assert(stream_rounded(2048,STREAM_CAP)==2048);assert(stream_rounded(2049,STREAM_CAP)==4096);
    assert(stream_rounded(STREAM_CAP,STREAM_CAP)==STREAM_CAP);
    assert(!stream_rounded(STREAM_CAP+1,STREAM_CAP));assert(!stream_rounded(0xffffffffu,STREAM_CAP));
    assert(!stream_rounded(2049,4095));assert(stream_rounded(2048,2048)==2048);
    for(int i=1;i<3;i++) {
        FILE *f=fopen(argv[i],"rb");assert(f);size_t length=fread(data,1,sizeof data,f);fclose(f);
        assert(valid((S32)length,1));assert(valid((S32)length,17));assert(valid((S32)length,128));assert(valid((S32)length,65535));
        StreamImage v;assert(stream_image_begin(&v,data,(S32)length));assert(stream_image_step(&v,data,0)==0 && v.offset==32);
        assert(!valid((S32)length-1,128));assert(!valid((S32)length+1,128));
        for(unsigned at=0;at<length;at+=31) {
            data[at]^=0x80;assert(!valid((S32)length,128));data[at]^=0x80;
        }
        assert(stream_image_begin(&v,data,(S32)length));
        /* Invalid tile reference is refused before a map can be published. */
        unsigned at=v.map_offset;unsigned char saved=data[at];data[at]=0xff;
        assert(!valid((S32)length,128));data[at]=saved;assert(valid((S32)length,128));
    }
    return 0;
}

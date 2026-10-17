#ifdef _WIN32
/* This host fixture uses portable C file I/O, not Microsoft-only fopen_s. */
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "asset.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    assert(argc==7);
    FILE *file=fopen(argv[1],"rb");assert(file);
    assert(!fseek(file,0,SEEK_END));long size=ftell(file);assert(size>=0 && size<80000);rewind(file);
    unsigned char *raw=malloc((size_t)size+1);assert(raw);
    assert(fread(raw+1,1,(size_t)size,file)==(size_t)size);fclose(file);
    struct {unsigned int lo;AssetView v;unsigned int hi;} guarded={.lo=0xDEADBEEF,.hi=0xA5A55A5A};
    unsigned short good=asset_open(raw+1,(unsigned int)size,&guarded.v);
    assert(good==atoi(argv[2]));
    assert(guarded.lo==0xDEADBEEF && guarded.hi==0xA5A55A5A);
    if(good) {
        assert(guarded.v.width==atoi(argv[3]) && guarded.v.height==atoi(argv[4]));
        assert(guarded.v.tiles==atoi(argv[5]) && guarded.v.entries==atoi(argv[6]));
    }
    assert(asset_crc((unsigned char const *)"123456789",9)==0xCBF43926u);
    free(raw);return 0;
}

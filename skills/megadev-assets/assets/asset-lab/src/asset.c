#include "asset.h"
_Static_assert(sizeof(unsigned short)==2 && sizeof(unsigned int)==4,"Asset decoder needs 16/32-bit types");
unsigned short asset_word(volatile unsigned char const *p) {
    return (unsigned short)((unsigned short)p[0]<<8|p[1]);
}
static unsigned int dword(volatile unsigned char const *p) {
    return (unsigned int)asset_word(p)<<16|asset_word(p+2);
}
unsigned int asset_crc(volatile unsigned char const *p,unsigned int length) {
    unsigned int crc=0xFFFFFFFFu;
    for(unsigned int i=0;i<length;i++) {
        crc^=p[i];
        for(unsigned short b=0;b<8;b++)crc=(crc>>1)^((0u-(crc&1u))&0xEDB88320u);
    }
    return ~crc;
}
/* Megadev links directly with ld, without an implicit C runtime/libgcc.
   Keep this bounded header arithmetic independent of optimization level. */
static unsigned int product(unsigned short a,unsigned short b) {
    unsigned int result=0,value=a;
    while(b) {if(b&1u)result+=value;value<<=1;b>>=1;}
    return result;
}
unsigned short asset_open(volatile unsigned char const *data,unsigned int available,AssetView *v) {
    if(available<64 || data[0]!='M' || data[1]!='D' || data[2]!='A' || data[3]!='1' || asset_word(data+4)!=1 || dword(data+28))return 0;
    v->kind=asset_word(data+6);v->width=asset_word(data+8);v->height=asset_word(data+10);
    v->tiles=asset_word(data+12);v->entries=asset_word(data+14);v->tile_bytes=dword(data+16);v->total=dword(data+20);
    if(v->kind>1 || !v->width || !v->height || !v->tiles || v->tiles>2048 || v->tile_bytes!=(unsigned int)v->tiles*32u || v->total!=available)return 0;
    if(!v->kind) {
        if(v->width>64 || v->height>32 || v->entries!=product(v->width,v->height))return 0;
    } else if(v->width>4 || v->height>4 || !v->entries || v->tiles!=product((unsigned short)product(v->width,v->height),v->entries))return 0;
    v->map_offset=64+v->tile_bytes;
    if(v->total!=v->map_offset+(v->kind?0u:(unsigned int)v->entries*2u))return 0;
    if(asset_crc(data+32,v->total-32)!=dword(data+24))return 0;
    for(unsigned short i=0;i<16;i++)if(asset_word(data+32+i*2)&0xF111u)return 0;
    if(!v->kind)for(unsigned short i=0;i<v->entries;i++) {
        unsigned short cell=asset_word(data+v->map_offset+i*2u);
        if((cell&0xE000u) || (cell&0x7FFu)>=v->tiles)return 0;
    }
    return 1;
}

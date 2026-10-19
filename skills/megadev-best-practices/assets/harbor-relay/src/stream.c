#include "stream.h"
_Static_assert(sizeof(S16)==2 && sizeof(S32)==4,"16/32-bit integer types required");
S16 stream_word(volatile unsigned char const *p){return (S16)((S16)p[0]<<8|p[1]);}
static S32 longword(volatile unsigned char const *p){return (S32)stream_word(p)<<16|stream_word(p+2);}
S32 stream_rounded(S32 bytes,S32 capacity) {
    if(!bytes || bytes>0xFFFFF800u)return 0;
    S32 rounded=(bytes+2047u)&~2047u;
    return rounded<=capacity?rounded:0;
}
int stream_image_begin(StreamImage *v,volatile unsigned char const *p,S32 length) {
    v->done=0;v->valid=0;v->offset=32;v->crc=0xFFFFFFFFu;
    if(length<64 || length>STREAM_CAP || p[0]!='M' || p[1]!='D' || p[2]!='A' || p[3]!='1' ||
       stream_word(p+4)!=1 || stream_word(p+6)!=0 || stream_word(p+8)!=40 || stream_word(p+10)!=20 ||
       stream_word(p+14)!=STREAM_MAP || longword(p+28))return 0;
    v->tiles=stream_word(p+12);
    if(!v->tiles || v->tiles>STREAM_TILES || longword(p+16)!=(S32)v->tiles*32u)return 0;
    v->map_offset=64u+(S32)v->tiles*32u;
    if(length!=v->map_offset+STREAM_MAP*2u || longword(p+20)!=length)return 0;
    for(S16 i=0;i<16;i++)if(stream_word(p+32u+i*2u)&0xF111u)return 0;
    v->length=length;v->wanted=longword(p+24);v->valid=1;return 1;
}
int stream_image_step(StreamImage *v,volatile unsigned char const *p,S16 budget) {
    if(!v->valid)return -1;
    if(v->done)return 1;
    if(budget>128)budget=128; /* Bounded foreground CRC work, separate from uploads. */
    while(budget-- && v->offset<v->length) {
        S32 n=v->offset;
        if(n>=v->map_offset && !(n&1u)) {
            S16 cell=stream_word(p+n);
            if((cell&0xE000u) || (cell&0x7FFu)>=v->tiles){v->valid=0;return -1;}
        }
        v->crc^=p[n];
        for(S16 bit=0;bit<8;bit++)v->crc=(v->crc>>1)^((0u-(v->crc&1u))&0xEDB88320u);
        v->offset++;
    }
    if(v->offset==v->length) {
        if(~v->crc!=v->wanted){v->valid=0;return -1;}
        v->done=1;return 1;
    }
    return 0;
}

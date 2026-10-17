#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "gfx.h"
#include <assert.h>
#include <string.h>
_Static_assert(sizeof(GfxU16)==2,"16-bit model word required");
_Static_assert(sizeof(GfxList)==1320 && sizeof(GfxState)==20,"CPU fixture layout changed");
int main(void) {
    struct {unsigned int lo;GfxList list;unsigned int hi;} guard={.lo=0xDEADBEEF,.hi=0xA5A55A5A};
    GfxList *s=&guard.list;
    gfx_begin(s,80);
    assert(!s->count && s->sat[0][0]==0 && s->sat[0][1]==0);
    assert(gfx_sprite(s,-32,0,32,24,288,1,0,0)==GFX_CULLED);
    assert(gfx_sprite(s,320,0,32,24,288,1,0,0)==GFX_CULLED);
    assert(gfx_sprite(s,-32768,32767,32,24,288,1,0,0)==GFX_CULLED);
    assert(gfx_sprite(s,-31,-23,32,24,288,1,0,1)==GFX_OK);
    assert(s->sat[0][0]==105 && s->sat[0][3]==97);
    assert(s->sat[0][1]==0x0E00 && s->sat[0][2]==0x2920);
    assert(s->entries[0]==1 && !s->entries[1]);
    assert(gfx_sprite(s,30,30,16,16,304,2,1,0)==GFX_OK);
    assert((s->sat[0][1]&127)==1 && (s->sat[1][1]&127)==0);
    unsigned short old=s->sat[1][1];
    assert(gfx_sprite(s,0,0,7,8,0,0,0,0)==GFX_BAD);
    assert(gfx_sprite(s,0,0,32,32,2040,0,0,0)==GFX_BAD);
    assert(gfx_sprite(s,0,0,8,8,2048,0,0,0)==GFX_BAD);
    assert(gfx_sprite(s,0,0,8,8,0,4,0,0)==GFX_BAD);
    assert(gfx_sprite(s,0,0,8,8,0,0,2,0)==GFX_BAD);
    assert(s->count==2 && s->sat[1][1]==old);
    gfx_begin(s,80);
    for(unsigned i=0;i<20;i++)assert(gfx_sprite(s,0,100,8,8,0,0,0,0)==GFX_OK);
    assert(gfx_sprite(s,40,100,8,8,0,0,0,0)==GFX_SCANLINE);
    assert(s->count==20 && s->entries[100]==20 && (s->sat[19][1]&127)==0);
    gfx_begin(s,80);
    for(unsigned i=0;i<10;i++)assert(gfx_sprite(s,0,0,32,8,0,0,0,0)==GFX_OK);
    assert(gfx_sprite(s,0,0,8,8,0,0,0,0)==GFX_SCANLINE);
    assert(gfx_sprite(s,0,8,8,8,0,0,0,0)==GFX_OK);
    gfx_begin(s,65535);assert(s->capacity==80);
    for(unsigned i=0;i<80;i++)assert(gfx_sprite(s,(GfxS16)((i%10)*32),(GfxS16)((i/10)*8),32,8,0,0,0,0)==GFX_OK);
    assert(gfx_sprite(s,0,150,8,8,0,0,0,0)==GFX_FULL);
    assert((s->sat[79][1]&127)==0);
    gfx_begin(s,0);assert(gfx_sprite(s,0,0,8,8,0,0,0,0)==GFX_FULL);
    assert(gfx_upload_bytes(80,80)==932 && gfx_upload_bytes(0,0)==140);
    assert(gfx_upload_bytes(81,0)==0 && gfx_upload_bytes(1,81)==0);
    assert(gfx_upload_bytes(24,80)<=GFX_UPLOAD_MAX);
    assert(gfx_frame(5,6,2)==0 && gfx_frame(6,6,2)==1 && gfx_frame(12,6,2)==0);
    assert(gfx_frame(65535,0,2)==0 && gfx_frame(65535,2,0)==0);
    assert(gfx_scroll(0,1)==0 && gfx_scroll(1,1)==1023 && gfx_scroll(512,1)==0 && gfx_scroll(511,4)==897 && gfx_scroll(3,0)==0);
    for(unsigned c=0;c<4096;c++) {
        GfxU16 color=(GfxU16)c&0x0EEE;
        assert(gfx_shade(color,0)==color && gfx_shade(color,-7)==0 && gfx_shade(color,7)==0x0EEE);
    }
    assert(gfx_shade(0x0EEE,-32768)==0 && gfx_shade(0,32767)==0x0EEE);
    GfxState a,b;gfx_reset(&a);gfx_reset(&b);
    for(unsigned i=0;i<60;i++)gfx_step(&a,8,60);
    for(unsigned i=0;i<50;i++)gfx_step(&b,8,50);
    assert(a.x==280 && a.tick==60 && !memcmp(&a,&b,sizeof(a)));
    GfxState old_state=a;a.paused=1;old_state.paused=1;
    assert(!gfx_step(&a,8,60) && !memcmp(&a,&old_state,sizeof(a)));
    a.paused=0;old_state=a;assert(!gfx_step(&a,8,0) && !memcmp(&a,&old_state,sizeof(a)));
    gfx_step(&a,15,60);assert(a.x==old_state.x && a.y==old_state.y);
    for(unsigned i=0;i<1000;i++)gfx_step(&a,10,60);
    assert(a.x==992 && a.y==194 && a.camera==704);
    for(unsigned i=0;i<1000;i++)gfx_step(&a,5,50);
    assert(a.x==2 && a.y==42 && a.camera==0 && a.flip==1);
    GfxU16 base[64],out[64];for(unsigned i=0;i<64;i++)base[i]=(GfxU16)(i*2)&0x0EEE;
    gfx_palette(base,out,0,0);assert(!memcmp(base,out,sizeof(base)));
    gfx_palette(base,out,10,1);assert(out[56]==base[57] && out[57]==base[58] && out[58]==base[56]);
    assert(out[0]==base[0] && out[55]==base[55] && out[59]==base[59]);
    gfx_palette(base,out,56,2);for(unsigned i=16;i<64;i++)assert(!out[i]);
    for(unsigned i=0;i<16;i++)assert(out[i]==base[i]);
    gfx_palette(base,out,0,3);for(unsigned i=16;i<64;i++)assert(out[i]==0x0EEE);
    gfx_palette(base,out,8,3);assert(!memcmp(base,out,sizeof(base)));
    assert(guard.lo==0xDEADBEEF && guard.hi==0xA5A55A5A);
    return 0;
}

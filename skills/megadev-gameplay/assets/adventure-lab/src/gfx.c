#include "gfx.h"
void gfx_begin(GfxList *list,GfxU16 capacity) {
    list->count=0;list->capacity=capacity>80?80:capacity;list->culled=0;list->rejected=0;
    for(GfxU16 y=0;y<GFX_LINES;y++){list->pixels[y]=0;list->entries[y]=0;}
    for(GfxU16 w=0;w<4;w++)list->sat[0][w]=0;
}
int gfx_sprite(GfxList *s,GfxS16 x,GfxS16 y,GfxU16 w,GfxU16 h,GfxU16 tile,GfxU16 pal,GfxU16 priority,GfxU16 flip) {
    if(w<8 || w>32 || h<8 || h>32 || (w&7) || (h&7) || tile>=2048 || pal>3 || priority>1 || flip>1){s->rejected++;return GFX_BAD;}
    GfxU16 cells=(GfxU16)((w>>3)*(h>>3));
    if(tile>2048-cells){s->rejected++;return GFX_BAD;}
    /* Test before adding offsets: extreme offscreen coordinates cannot wrap. */
    if(x<=-(GfxS16)w || x>=320 || y<=-(GfxS16)h || y>=224){s->culled++;return GFX_CULLED;}
    if(s->count>=s->capacity){s->rejected++;return GFX_FULL;}
    GfxU16 top=y<0?0:(GfxU16)y,bottom=y+h>224?224:(GfxU16)(y+h);
    /* Conservative: charge the entire sprite width, including transparent pixels. */
    for(GfxU16 line=top;line<bottom;line++)if(s->entries[line]>=20 || s->pixels[line]+w>320){s->rejected++;return GFX_SCANLINE;}
    GfxU16 n=s->count;
    s->sat[n][0]=(GfxU16)(y+128);
    s->sat[n][1]=(GfxU16)((((w>>3)-1)<<10)|(((h>>3)-1)<<8));
    s->sat[n][2]=(GfxU16)(tile|(pal<<13)|(priority<<15)|(flip<<11));
    s->sat[n][3]=(GfxU16)(x+128);
    if(n)s->sat[n-1][1]|=n; /* Hardware follows links, not count. Last link stays zero. */
    for(GfxU16 line=top;line<bottom;line++){s->entries[line]++;s->pixels[line]+=w;}
    s->count++;return GFX_OK;
}
GfxU16 gfx_frame(GfxU16 tick,GfxU16 hold,GfxU16 frames) {
    return hold && frames?(GfxU16)((tick/hold)%frames):0;
}
GfxU16 gfx_shade(GfxU16 color,GfxS16 amount) {
    if(amount>7)amount=7;
    if(amount<-7)amount=-7;
    GfxU16 result=0;
    for(GfxU16 shift=1;shift<=9;shift+=4) {
        GfxS16 channel=(GfxS16)((color>>shift)&7)+amount;
        if(channel<0)channel=0;
        if(channel>7)channel=7;
        result|=(GfxU16)((GfxU16)channel<<shift);
    }
    return result;
}
GfxU16 gfx_scroll(GfxU16 camera,GfxU16 divisor) {
    /* The Lab repeats a 512-pixel source, even though its plane is 512px wide. */
    if(!divisor)return 0;
    return (GfxU16)(0u-((camera/divisor)&511u))&0x3FFu;
}
GfxU16 gfx_upload_bytes(GfxU16 count,GfxU16 hud_words) {
    if(count>80 || hud_words>80)return 0;
    return (GfxU16)((count?count:1)*8u+128u+4u+hud_words*2u);
}
void gfx_reset(GfxState *s) {
    s->x=160;s->y=116;s->camera=16;s->tick=0;s->remainder=0;s->paused=0;s->parallax=1;s->effect=0;s->front=0;s->flip=0;
}
GfxU16 gfx_step(GfxState *s,GfxU16 held,GfxU16 hz) {
    if(s->paused || (hz!=50 && hz!=60))return 0;
    GfxU16 steps=0;s->remainder+=60;
    while(s->remainder>=hz) {
        s->remainder-=hz;s->tick++;steps++;
        if((held&12)==8){if(s->x<992)s->x+=2;s->flip=0;}
        if((held&12)==4){if(s->x>2)s->x-=2;s->flip=1;}
        if((held&3)==1 && s->y>42)s->y-=2;
        if((held&3)==2 && s->y<194)s->y+=2;
        s->camera=s->x<144?0:(s->x>848?704:(GfxU16)(s->x-144));
    }
    return steps;
}
void gfx_palette(GfxU16 const base[64],GfxU16 out[64],GfxU16 tick,GfxU16 effect) {
    for(GfxU16 i=0;i<64;i++)out[i]=base[i];
    if(effect==1) {
        /* Rotate three reserved warm colors on the quay line only. */
        GfxU16 phase=(GfxU16)((tick/10)%3);
        for(GfxU16 i=0;i<3;i++)out[56+i]=base[56+(GfxU16)(i+phase)%3u];
    } else if(effect==2) {
        GfxU16 phase=(GfxU16)((tick/8)&15);GfxS16 dark=(GfxS16)(phase<8?phase:15-phase);
        for(GfxU16 i=16;i<64;i++)out[i]=gfx_shade(base[i],-dark);
    } else if(effect==3) {
        GfxU16 phase=(GfxU16)(tick%120);GfxS16 light=phase<8?(GfxS16)(7-phase):0;
        for(GfxU16 i=16;i<64;i++)out[i]=gfx_shade(base[i],light);
    }
}

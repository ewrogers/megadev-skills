#ifndef GRAPHICS_LAB_GFX_H
#define GRAPHICS_LAB_GFX_H
/* Portable model: no VDP, BIOS, allocations or compiler-specific bitfields. */
typedef unsigned short GfxU16;
typedef signed short GfxS16;
#define GFX_SPRITES 80
#define GFX_LINES 224
#define GFX_UPLOAD_MAX 1024
/* Eight-byte SAT entries, in native u16 words for the target backend. */
typedef struct {
    GfxU16 sat[GFX_SPRITES][4];
    GfxU16 pixels[GFX_LINES];
    unsigned char entries[GFX_LINES];
    GfxU16 count,capacity,culled,rejected;
} GfxList;
typedef struct {
    GfxU16 x,y,camera,tick,remainder,paused,parallax,effect,front,flip;
} GfxState;
enum { GFX_OK=1,GFX_CULLED=0,GFX_BAD=-1,GFX_FULL=-2,GFX_SCANLINE=-3 };
void gfx_begin(GfxList *list,GfxU16 capacity);
int gfx_sprite(GfxList *list,GfxS16 x,GfxS16 y,GfxU16 width,GfxU16 height,GfxU16 tile,GfxU16 palette,GfxU16 priority,GfxU16 flip);
GfxU16 gfx_frame(GfxU16 tick,GfxU16 hold,GfxU16 frames);
GfxU16 gfx_shade(GfxU16 color,GfxS16 amount);
GfxU16 gfx_scroll(GfxU16 camera,GfxU16 divisor);
GfxU16 gfx_upload_bytes(GfxU16 count,GfxU16 hud_words);
void gfx_reset(GfxState *state);
GfxU16 gfx_step(GfxState *state,GfxU16 held,GfxU16 hz);
void gfx_palette(GfxU16 const base[64],GfxU16 out[64],GfxU16 tick,GfxU16 effect);
#endif

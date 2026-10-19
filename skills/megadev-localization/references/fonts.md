# Native glyphs, patterns and residency

## Verify Megadev's actual text behavior

Audited source: Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, from the pulled
`ewrogers/megadev:latest` image.

- [BIOS C helpers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h):
  `bios_print` documents ASCII bytes, FF termination, zero newline,
  `BIOS_FONT_TILE_BASE` addition and palette line 0. It is not a UTF-8 decoder.
- [BIOS definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h) and
  [memory map](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/memmap.h):
  inspect font/register/palette buffers and the exact upload ABI before changing
  the renderer. The Lab uses a C ABI adapter for `BIOS_DMA_XFER` from Main Work
  RAM and its own tilemap renderer. Its private codec is not BIOS-print input.

There are no SGDK `VDP_drawText`, `PAL_*` or ResComp calls in this pipeline.
The renderer preserves the BIOS VDP register cache when changing page bases.

## Repertoire and visual quality

Gather the real strings and allowed names first, including punctuation, numbers,
menu actions and fallback messages. Normalize consistently and validate every
required character against a deliberately authored font. Use distinct native
accented/lowercase forms; do not replace accented letters with base letters or
scale an unrelated large font down silently. Style and pixel dimensions come
from the game brief, not Language Lab.

The Lab's original 8×8 Latin font includes 117 glyphs: ASCII plus the accents
needed by four example locales and its name grid. It draws accents as actual
bitmap features. It does not provide a full European repertoire, Japanese font,
right-to-left shaping or grapheme-aware arbitrary Unicode name editing.

An 8×8 4bpp VDP pattern costs 32 bytes. A 16×16 glyph can cost four patterns,
not one. Count unique loaded patterns, palette entries, tilemap references and
transfer bytes separately from source file bytes. Dithering is not a remedy for
an unreadable glyph; use crisp native shapes and adequate contrast.

## Disc and RAM are separate budgets

The Lab reserves 192 tiles (6,144 VRAM bytes) for one shared Latin atlas. Of
those, 117 patterns (3,744 bytes) carry authored glyphs; the remaining 75 slots
are padded/reserved. The uploaded/source atlas is still 6,144 bytes. The font
budget page and generated report expose used patterns versus reservation.

This is standard CD boot with a resident Main module. Embedded const catalogs,
font source patterns, icons and portraits occupy Main Work RAM along with code;
they are not free cartridge-ROM reads. The initial font upload does not reclaim
that resident module storage. BSS, stack and layout scratch are additional costs.
The example deliberately keeps all four small text catalogs and the shared font
resident, avoiding a new drive/Word RAM protocol for a few kilobytes of text.

For larger repertoires, choose resident shared glyphs, locale/scene banks or a
bounded glyph cache. Coordinate CD preloading, Word RAM ownership and Main
staging with the existing streaming/memory owners. A bank change must keep the
old page/font valid until the new page and its matching glyph patterns are ready.
Never display a tilemap whose glyph IDs refer to the previous bank.

Lab VRAM byte ranges: font 0400..1BFF, panels 1C00..1D9F, icons 1E00..21FF,
portrait 2200..267F, backdrop 2680..26FF; Plane B A000, SAT B800, HScroll BC00,
Plane A pages C000 and E000 (4 KiB each). Four palettes are shared with the UI.
Resource addresses and capacities are example reservations, not required layouts
for another game. Real games must reserve these resources alongside world art.

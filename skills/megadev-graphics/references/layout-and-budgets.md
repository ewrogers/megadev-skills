# Layout and budgets

## Verify the selected source

The tested container uses Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Public commands use `ewrogers/megadev:latest`; inspect `doctor` and the selected source again when adapting this example.

- [VDP registers and modes](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.def.h): commands, scroll modes and register fields.
- [VDP types and address conversion](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.h): `to_vdp_addr`, `VDP_PLANE_POS` and port definitions.
- [BIOS constants](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h) and [wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h): default tables, caches, callbacks and DMA source variants.
- [Upstream starter](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project): startup and Main/Sub loading conventions.

## Count the actual hardware work

The Lab uses H40, 320×224, non-interlaced display. It has 64 KiB of VRAM and four 16-entry CRAM palette lines. Each 8×8 4bpp pattern takes 32 bytes; name tables, sprite tables and scroll tables share the same VRAM. Do not advertise all 2048 patterns as freely available art slots.

H40 allows 80 hardware sprites in the frame, with 20 sprite entries and 320 sprite pixels on a scanline. These are separate limits. Budget complete sprite widths, including transparent art, rather than counting only opaque pixels. H32 instead uses 64 / 16 / 256; the supplied model is deliberately H40-only. Splitting a large actor increases entry and scanline costs. Offscreen culling is useful but does not solve overlap within the screen.

`gfx_sprite` validates 8/16/24/32-pixel hardware dimensions, palette and priority bits, pattern extent and coordinates before adding the hardware coordinate bias. It preserves accepted entries when another candidate is rejected. Linked SAT entries terminate with link zero; an empty list has one hidden terminator. The controlled actor is submitted first. The example conservatively refuses a candidate that would exceed either scanline allowance; it is not an optimal packing algorithm or a replacement for art composition.

## Graphics Lab allocation

| VRAM bytes | Use |
| --- | --- |
| 0400–0FFF | Original 96-glyph opaque font, ASCII 32–127 |
| 2400–26FF | Two 32×24 ship frames, 24 patterns |
| 3000–6A9F | Sky, 469 deduplicated patterns |
| 6AA0–8CBF | Quay, 273 deduplicated patterns |
| A000–AFFF | BIOS Window table reservation |
| B800–BA7F | 80-entry sprite table reservation |
| BC00–BFFF | Scroll table reservation; full-plane mode uses four bytes |
| C000–DFFF | 64×64 Plane A table |
| E000–FFFF | 64×64 Plane B table |

Unused gaps are left unused deliberately. The scene loader rejects any pattern range reaching the table region at A000. The HUD uses only the Window's top four rows, but its table reservation and H40 stride still matter. A different plane size or scroll mode needs a new allocation check.

Palette line 0 serves the opaque font/HUD, line 1 the ships, line 2 the distant sky and line 3 the quay. Compatible ship/quay source colors are deliberately assigned separate lines so effects can be isolated. This is an example allocation, not a mandatory palette arrangement.

Resident Main code/data uses FF0000–FF9FFF; mutable graphics state uses FFA000–FFB7FF. The unused room-module compatibility fixture has a separate FFB800–FFBFFF state range. Main's stack is FFD000–FFDFFF; BIOS-owned Work RAM begins at FFF700. The double-buffered frame data is part of this RAM budget. Loading uses Main/Sub-owned 2M Word RAM in sequence; no second CPU writes the VDP.

## Byte budgets are not timing measurements

The callback transfers at most 24 SAT entries (192 bytes), 128 palette bytes, four scroll bytes and 160 HUD bytes: **484 data-port bytes**. Offscreen actors lower that count. The generic model also tests 80-entry bounds, but the Lab's runtime cap is 24. Control-port commands, BIOS work, CPU loop overhead, FIFO waits and other subsystems cost time too. `UPLOAD` counts submitted data bytes, not CPU percentage, FPS or a universal safe DMA allowance.

Both disc backgrounds load at startup with display off. There are no per-frame pattern uploads. Future animation uploads, audio, line scrolling or streaming must share a measured budget; do not assume this demonstration leaves the same headroom in a different game. Real hardware and PAL/NTSC timing checks remain necessary.

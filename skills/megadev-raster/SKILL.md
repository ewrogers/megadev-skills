---
name: megadev-raster
description: Implement bounded HBlank water/reflection and split-screen effects in Megadev Sega CD games, preserving VBlank ownership, interrupt safety and palette/scroll budgets.
---

# Megadev raster effects

Use raster effects when the design needs different display state within a frame.
Mega CD still uses the Mega Drive VDP: extra CPUs, Word RAM and the ASIC do not
add palettes, planes, sprite capacity or a second independent display.
The Labs' scenery, audio, controls and split sizes are illustrative.

- Read [Interrupt ownership and timing](references/interrupts.md) before installing
  a handler or changing VDP state during display.
- Read [Water and split-screen](references/rendering.md) for reflected tilemaps,
  line scroll, palette pairs, camera transforms and seam/sprite budgets.
- Read [Validation](references/testing.md) for failure tests, regional builds,
  emulator checks and physical-hardware limits.

Inspect actual source in `ewrogers/megadev:latest`. The Lab's `rt_*` functions and
`raster_*` assembly are application code, not Megadev raster APIs. Do not import
SGDK interrupt, DMA-queue or sprite-engine calls.

Essential rules:

- Reuse the game's VBlank, input, audio and Main/Sub service owners. One owner
  schedules VDP access; an H handler must not compete with foreground uploads.
- Build an immutable frame outside interrupts. Commit its line, scroll, palette
  and seam sprites together, then latch a dedicated HBlank plan in VBlank.
- Restore dry/top state early in VBlank. Program the next counter before bulk
  uploads, finish within a measured blank interval, then arm the current epoch.
  Reject pending blank-edge/early interrupts without consuming that plan.
- Keep HBlank assembly short and bounded. Save used registers, return with RTE,
  and perform no BIOS, C, allocator, CD or decoder calls there. Bound edge polling
  and interrupt masking; reject late writes rather than coloring another frame.
- Minimize active-display CRAM writes. Prefer prepared palette banks and line
  scroll; if needed, preload a tiny color pair and write on a fresh HBlank.
  A foam sprite can hide the seam, but cannot excuse an unbounded palette upload.
- Split-screen shares planes, SAT, palettes and VRAM. Transform and clip sprites
  for each view, budget scanlines, and explicitly reserve divider rows.
- Measure repeated movement and boundary cases, not just a fixed waterline.
  Emulator success does not prove CRAM-dot absence or interrupt/bus timing on
  physical hardware. Offer an effect-off comparison and a simpler fallback.

[Raster Labs](assets/raster-lab/README.md) generate separate water and split discs
with `scripts/create_project.py --demo water|split`. The renderer uploads resident
art once and bounds steady-state DMA. Reuse `megadev-graphics` for general VDP
rendering, `megadev-system` for Main/Sub work, `megadev-audio` for audio ownership
and `megadev-performance`/`megadev-hardware` for broader qualification. This skill
owns scanline scheduling and the within-frame transforms, not those systems.

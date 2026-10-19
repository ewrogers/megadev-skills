# APIs and validation

Developed against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` and GCC 16.2.0.
Public commands use `ewrogers/megadev:latest`: pull, run `doctor`, then check the
selected headers again when the image changes.

Primary sources in [Megadev](https://github.com/drojaazu/megadev):

- [bios.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h): display, DMA, pad polling and Sub INT2 BIOS entry points.
- [bios.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h): register/palette caches and joypad lvalues.
- [vdp.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.h) and [vdp.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.def.h): ports, command encoding and VBlank status.
- [io.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.def.h) and [build.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/build.def.h): pad bits and video constants.

`ui_*`, `UiModel`, `UiPainter`, `UiBatch`, `ui_install` and `lab_upload_work` are
example facilities, not SDK APIs. Assembly adapters preserve C callee-saved
registers around BIOS calls. Upload sources are Main Work RAM, not Word RAM.
The IRQ wrapper preserves all data/address registers it uses and returns with
RTE. The foreground loop explicitly runs with interrupts enabled; callers with
a different interrupt policy need an appropriate adapter.

## Checks

- Standalone host `tools/check_ui.py`: O0/O3, warnings as errors, UBSan on Unix;
  bounds rejection without partial writes, shrinking/overflowing counters,
  restored focus, stack bounds, action edges and PAL/NTSC repeat timing.
- Original-MC68000 execution: all pages, invalid states, immutable in-flight
  batches, late/active-display deferral, exact row bytes/stride, final-only map
  switching, BIOS cache consistency, idle and malformed publication handling.
- ARM64/AMD64: fresh US/JP NTSC, EU PAL, debug and tour builds; compare payloads.
  The compiled tour is executed until all eight pages have been committed.
- Reproducible native art/palettes, standalone installation with telemetry off,
  and eight PNG previews rendered from the actual C tilemaps.

In ares, try title → Journey, coins/HP, Pack → Details/Character → Back, pause,
Settings from both routes, font/pulse changes, title and Credits. Check clipping,
stale glyphs, page tearing and CRAM dots. Repeat PAL with an authorized BIOS.
The optional tour is a convenience, not proof a person checked every screen.
MMIO capture is not a VDP model. Physical margins, pad latency, BIOS revisions
and real transfer timing still need console checks; no console result is claimed.

The user confirmed navigation, display and settings in ares for NTSC and PAL.

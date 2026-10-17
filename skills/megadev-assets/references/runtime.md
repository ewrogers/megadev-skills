# Disc, residency and VDP uploads

## Keep the lifetimes distinct

Upstream Megadev demonstrates [assembly incbin resources](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project/src/ex1_res.s) and a [Word RAM upload](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project/src/ex1.c). It does not interpret SGDK ResComp declarations or this example's MDA1 header automatically.

Asset Lab embeds the shared balloon and panel in resident Main code/data, then loads GARDEN.MDA and CAVERN.MDA from the disc through Sub into 2M Word RAM. Main validates a prepared asset only after successful completion and confirmed ownership. It retains that buffer until all reads/uploads finish. Subsequent scene loads reuse the same Word RAM and VRAM region; the shared assets are not loaded again.

Sub's disc filename table, actual disc files and Main's expected file lengths must agree. The build verifies prepared-file hashes and generates the expected lengths from those reports. Source edits fail the build until deliberately re-prepared. Modules/imports still use the project skill's startup and immutable-disc contracts.

## VDP transfer contracts

A DMA length here is in **16-bit words**, not bytes. Pattern payloads are even-sized and aligned at offset 64. Use the ordinary BIOS DMA service for resident Main RAM and the Word RAM-specific service for a Word RAM source.

Megadev's [BIOS header](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h) documents `bios_dma_xfer_word_ram`: Word RAM DMA needs a final-word data-port write, handled by that BIOS routine. Do not replace it with the ordinary source variant merely because both use a pointer. The Lab uses out-of-line C ABI adapters to preserve callee-saved registers around BIOS calls; a mocked CPU test is not a real-BIOS implementation test.

The Lab hides rendering during bulk pattern/map replacement, updates the palette cache, waits for a BIOS VBlank update and shows the display again. Direct VDP control/data writes are serialized against interrupts, and the cached backdrop register is kept consistent with the hardware register. This is a simple loading transition, not seamless streaming or a general DMA scheduler.

The mapper adds a scene VRAM base and a palette line to validated relative indices and flips. A background's row stride comes from the configured plane width, not its PNG width. In this BIOS setup that is 64 cells / 128 bytes per row.

## Example VRAM and palettes

| Range | Use |
| --- | --- |
| 0000–0FFF | Reserved low patterns/font area |
| 3000–317F | Shared two-frame 16×24 sprite (12 patterns) |
| 3200–321F | Shared panel pattern |
| 4000–9FFF | Reusable scene pattern region (768 patterns) |
| A000–AFFF | Reserved BIOS window table |
| B800 and BC00 regions | BIOS sprite table and scroll table reservations |
| C000–DFFF | Plane A table |
| E000–FFFF | Plane B table |

These are this Lab's reservations, not universal defaults to copy blindly. Check [BIOS layout constants](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h), the actual configured table sizes/alignment and all unused gaps before changing them.

Palette line 0 serves the font; line 1 is shared by the balloon and panel; line 2 changes with the location. Line 3 stays free. The Lab verifies that the panel and sprite palettes agree. Sharing compatible palettes conserves lines; it is not a rule to force unrelated assets together.

Opaque panel cells are present behind transparent font pixels on the other plane too. Replacing a foreground panel cell with a font tile otherwise exposes the scene backdrop, making text backgrounds change with a scene palette swap. The bottom background selects its own index-zero color as the global backdrop under the explicit preparation contract.

## Evidence and boundaries

The automatic tour loads both scenes twice with CRC/header validation, retaining the shared resources. Inspect both scenes and the animated sprite in ares; synthetic controller events may not reach its normal input path. The tour proves runtime operations in that emulator, not physical controller input.

Real hardware remains necessary for BIOS/model variation, actual disc latency/errors, Word RAM DMA quirks and timing under other workloads. This Lab does not qualify CD-DA/PCM coexistence, streaming throughput, raster effects or maximum sprite load. Artist source/used previews are idealized; filters and analog output can alter perceived colors.

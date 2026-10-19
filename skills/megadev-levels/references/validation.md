# Validation

Run `python -m unittest discover -s tests -v` from the repository. A generated Lab also provides `python tools/check_levels.py`, which uses a host C17 compiler at O0/O3 with undefined-behavior sanitization where supported.

Coverage includes:

- Exact native atlas preparation, palette/alpha rejection, duplicate/invalid Tiled data, GID flags, unsupported geometry/layers and missing exit targets.
- Source/output hash verification, stale/corrupt prepared data and preserving edited Tiled sources during normal build steps.
- Independent metatile/quadrant/flip expectations and a simulated two-plane VRAM cache through horizontal/vertical wraps in both directions. Every visible cell is checked after each update.
- Checked binary lengths/directories, collision foot boxes, bounded walking, camera clamping, named area travel, invalid spawns, pause and equal-time PAL/NTSC state.
- Original-68000 execution of the actual parser, model, cache, tour and publication callback: exact words, matching camera, late-frame retention, invalid run rejection and no foreground VDP writes during walking.
- Explicit H64/V32 VDP configuration and transparent empty foreground tiles. Both address real presentation faults found in the first ares build.

The Docker matrix builds US/JP NTSC, EU PAL, US debug and automatic tour on ARM64 and AMD64. It checks startup/interrupt bridges, disc contents and linked reservations and compares complete payload hashes/results. Local AMD64 on Apple Silicon is emulated; CI repeats on native runners. Installation checks disable telemetry.

Ares review should cover diagonal movement and both camera axes, C's tour positions, B's independent collision view, walking under foreground canopy tiles, A at both waystones and pause. Check the entire viewport after cache wrapping—not just the spawn frame—and ensure no missing rows, stale tiles, text flicker or CRAM dots. Repeat PAL with the European BIOS.

A CPU instruction model is not a VDP, CD drive or cycle-accurate hardware measurement. The cache oracle proves address/data selection within this profile, not FIFO or raster timing. Real-console cadence, BIOS variants, combined audio/CD/DMA load and larger authoring profiles require separate acceptance. Record those limits instead of claiming emulator success qualifies all hardware workloads.

On 2026-10-19 the user confirmed the corrected NTSC build's movement, camera tours,
collision view and connected-area travel in ares. The final PAL build's scrolling, camera tour, collision view, travel and pause
were confirmed the same day, without missing background rows. These reviews
are separate from compiled PAL/NTSC simulation equivalence.

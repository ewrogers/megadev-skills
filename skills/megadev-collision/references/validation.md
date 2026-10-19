# Validation

Run host tests from the repository with `python -m unittest discover -s tests -v`.
The installed Lab also provides `python tools/check_collision.py`; it needs a C17
host compiler. Neither check needs Docker, a BIOS or a ROM download.

## What the checks establish

- O0/O3 host contracts with undefined-behavior sanitization where supported:
  relative sweeps, strict overlap, boundary motion, corner grazing, exact nearest
  time, tied walls, bounded slides and invalid geometry/candidate lists.
- 12,000 seeded queries against an independent Python `Fraction` slab oracle,
  checking kind, fraction, contact time and blocked axes.
- PAL/NTSC clock equivalence, pause, catch-up cap, discrete tunneling versus swept
  stopping, raised walls and a 3,000-step slide route that never enters solids.
- Fresh standalone generation, refusal to replace existing folders, deterministic
  art, valid VDP palette words and agreement between wall pixels and geometry.
- Original-68000 execution of 1,200 independent oracle cases per target build,
  pointer interop, exact sliding, clock/pause/catch-up and the automatic tour.
- Executed VBlank publication: immutable frames, late-frame hold, bounded SAT/HUD
  bytes, no runtime CRAM write, idle no-op and fail-closed invalid counts. The
  production movement update has no foreground VDP writes; wall changes stage
  one column without display blanking and retain it across late VBlanks.

The matrix builds US/JP NTSC, EU PAL, US debug and an automatic tour through
`ewrogers/megadev:latest` on ARM64 and AMD64. It checks startup/interrupt bridges,
linked payloads and disc contents, rejects linker overflow, and requires identical
payload hashes/results across architectures. Local AMD64 on Apple Silicon is
emulated; CI uses native runners for both architectures. `tests/compare_reports.py`
rejects missing cases and missing collision/publication execution evidence.

## Presentation and physical checks

On 2026-10-19 the user confirmed the redraw fix in NTSC and the final PAL build
with all three views, aiming/sliding, wall changes and pause working.

In ares, review all three views on NTSC and PAL: a fast pink shot should tunnel,
blue should stop at the wall, rays should clip at the first contact and fast
movement should slide without entering the walls. Check pause, view changes,
contact markers, stable HUD text, sound and absence of CRAM dots.

The instruction model is not a VDP, CD drive or cycle-accurate machine. It does
not establish FPS, real-console frame cost or IRQ safety under every workload.
Repeat the worst combined workload on real hardware before assigning a game's
collision budget. Record the selected SDK/image and distinguish host tests,
target instruction execution, emulator presentation and physical-console results.

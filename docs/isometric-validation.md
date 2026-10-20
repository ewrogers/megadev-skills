# Isometric milestone validation

`megadev-isometric` connects portable projection/surface/order rules to the existing
Megadev platform contracts. Lantern Conservatory is an illustrative scene, not a
prescribed visual style or level design. Pure coordinate/ordering techniques and
procedural artwork were adapted from the repository family's earlier isometric
study; no SGDK runtime, resource compiler or sprite APIs are used.

## Reproduce

```sh
python -m unittest discover -s tests -p test_isometric.py
python tests/integration.py --skill megadev-isometric --platform linux/arm64 --output build/isometric-arm64
python tests/integration.py --skill megadev-isometric --platform linux/amd64 --output build/isometric-amd64
python tests/compare_reports.py build/isometric-arm64/report.json build/isometric-amd64/report.json
```

Each native matrix builds US/JP/EU release, US debug and qualification cases with
`ewrogers/megadev:latest`, `--cpus=2`. Reports record actual SDK/compiler revisions;
complete target modules and startup checks must match across architectures.
Qualification runs the executable contracts before normal play, without scripted
player input. Standalone installation checks disable telemetry.

Host checks include 6,000 independently projected convex-hull pairs, independent
DAG ordering, floor/inverse projection, elevation relationships, ambiguous/cyclic
and malformed inputs, cached/full equivalence, stair reversal, upper edges,
underpasses, collision footprints and PAL/NTSC/pause/catch-up behavior. Native
asset tests reconstruct all original ground pixels through packed tiles/maps and
reject malformed packet streams.

Original-M68000 tests execute contracts and exact art decode, then inspect actual
SAT/HUD/camera writes. They verify immutable frames, late-VBlank deferral, invalid
indices/counts, no normal CRAM writes and no authored tour admission rejection.
The Word RAM DMA ABI adapter must call BIOS_DMA_XFER_WORD_RAM (0x2D4 in Mode 2),
pass the correct arguments and preserve callee-saved registers. The initial wrong
ordinary-RAM DMA path caused visibly corrupted art in ares despite correct decode
bytes; this regression distinguishes byte/model evidence from real rendering.

## Budgets and limits

- 368 sprite patterns (11,776 bytes), 272 deduplicated ground patterns (8,704
  bytes), a 64×32 map (4,096 bytes), 96 font patterns and four palettes.
- Resident Main copies use bounded literal/repeat packets: actors 8,792 bytes,
  ground 6,223 bytes, map 1,446 bytes. Startup scratch is the first 16 KiB of
  Main-owned 2M Word RAM; no gameplay task shares it in this Lab.
- Pattern regions and scratch bounds are compile-time checked against the chosen
  BIOS VRAM layout. Two 1,644-byte immutable frames carry SAT/HUD/scroll state.
- Fifteen static order nodes plus one hero, within the 24-item/24-link hard caps;
  the M68000 static cache is 436 bytes. CPU tests check Main BSS/stack separation.

Ares PAL/NTSC results are recorded in the PR, including art, camera alignment,
above/below ordering, cutaway, animation, sound and pause. CPU register models do
not qualify pixels, cycles or sound. Physical-console acceptance remains needed
for Word RAM DMA, controller behavior and the combined rendering/audio workload.

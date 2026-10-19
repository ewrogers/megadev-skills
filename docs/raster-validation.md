# Raster milestone validation

The `megadev-raster` skill adds two standalone examples: Water Lab (moving
reflection/wave/tint) and Split Lab (two independently transformed views).
They share resident native art and one VBlank/HBlank owner. Their scenery,
controls, audio and camera layout are illustrative.

## Automated evidence

- Host suite: 194 tests pass, including eight raster tests. These check PAL/NTSC
  clock equivalence, pause, reflection geometry, coherent reflected actor cells,
  both cameras, sprite scanlines, invalid plans, native assets and standalone
  generation. Long O0/O3 runs use UBSan where supported.
- Both architecture profiles build ten cases: US/JP/EU release, US debug and US
  tour for each demo. Actual original-68000 code executes the model, immutable
  publisher, VBlank DMA adapter, H handler and vector lifecycle. Tests cover
  pending/early/late interrupts, stuck HBlank flags, epoch preemption, register
  preservation, recovery and measurement-window behavior. The comparison rejects
  missing cases or missing executed-raster/HBlank/probe evidence.
- The audited image reports Megadev
  `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0 and Binutils
  2.47.20260726. Public commands use `ewrogers/megadev:latest`; Docker containers
  have `--cpus=2`. Local ARM64 is native, local AMD64 is emulated. CI runs both
  architectures natively and compares target payloads.
- The disposable telemetry-disabled installation finds all 28 skills and creates
  both demos from the installed package. README and skills.sh Advanced group
  include raster. CI does not refresh registry telemetry.

Reproduce with:

```sh
python tests/integration.py --skill megadev-raster --platform linux/arm64 --output build/raster-arm64
python tests/integration.py --skill megadev-raster --platform linux/amd64 --output build/raster-amd64
python tests/compare_reports.py build/raster-arm64/report.json build/raster-amd64/report.json
python -m unittest discover -s tests
python tests/distribution.py
```

## Moving-water failure and correction

The initial demo could pass a fixed-line screenshot yet flicker and lose its
reflection after moving the surface, with every switch reported late. The
reproduction exposed late counter programming/VBlank overrun and a pending
blank-edge interrupt consuming the next frame's plan.

The corrected path restores dry/top state early, programs the counter before
bulk work, DMA-uploads scroll/SAT/HUD from immutable Main Work RAM snapshots and
arms only inside VBlank. Early, blank-edge and VBlank H requests leave that arm
intact. Each accepted H request is bounded and epoch-checked before a second
palette batch. No full-bank active-display palette copy is used.

A sustained NTSC automatic sweep completed more than 6,800 boundary switches
with matching completed requests, zero late windows and zero edge timeouts.
Setup finished around V-counter 234..235, versus the old path reaching visible
lines. The user confirmed repeated manual surface movement no longer flickered
or lost reflection. A fixed startup setup rejection remained stable rather than
increasing. Final data bounds include the separately projected ball reflection:
up to 18 SAT entries, 1,560 data bytes/frame and two 1,574-byte RAM snapshots.
The reflected ball uses one shared wave offset; independently offsetting its
8×8 cells produced visibly sliding halves and is now regression-tested.

These observations qualify this workload, not an arbitrary game's CPU margin.

## Evidence limits

The CPU harness models VDP status/scanlines and BIOS DMA. It proves contracts,
not bus cycles or a rendered screen. The user also confirmed PAL water controls, automatic sweeping, ball reflection
and pause, with the display at 50 FPS and no increasing L/T counters. The user
confirmed NTSC split controls/divider/HUD at 60 FPS, then PAL at 50 FPS including
panning, selection, tour, pause and the divider-free single-view comparison. Physical machines still need CRAM-dot,
FIFO/HBlank, short level-7 masking, VBlank budget, BIOS/model and combined
CD/audio/ASIC contention checks. See the skill's testing reference for the
manual sweep, tint, split-camera and pause checklist.

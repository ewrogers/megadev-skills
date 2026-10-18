# Scene validation

Test the lifecycle at its boundaries, not just the happy path:

- Pause/resume without stopping the Sub/CD service or losing input edges.
- Leaving during preload, validation, upload and an outstanding presentation.
- Missing/corrupt target data; preserve old local progress until commit succeeds.
- Repeated/latest requests; no second read overwrites the first request.
- A late callback with an old generation cannot release or modify a new scene.
- Stalled ownership and lost acknowledgments report a wait instead of freeing live data.
- Repeated cycles return to the same bounded resource use. Shared assets persist.
- Same-duration fades and cleanup at 50/60 Hz, with arithmetic and counter bounds.

The repository runs portable policy fixtures at O0/O3 with undefined-behavior
checking where supported. Executed original-68000 tests cover the actual policy
and coordinator, including a live old reader, unretired mailbox, held scene
generation and map-consumption acknowledgment. Existing audio and paired-reader
tests run against this Lab too, covering PCM, BIOS command ordering, failed
reads, Word RAM ownership, CRC checks and bounded VBlank payloads.

These are models, not a complete BIOS/VDP/drive simulator. The coordinator test
supplies completion events; the separate reader/publisher tests execute those
adapters. Do not present their combination as arbitrary instruction-interleaving
or physical seek-timing proof.

US/JP/EU release, US debug and US automatic tour build on native ARM64/AMD64 CI.
Compare target files, startup facts, executed scene checks and external CD-audio
WAV hashes; timestamped ISO bytes alone are not the comparison. Containers use
two CPUs. Installed generators run without repository helpers; CI disables telemetry.

## Ares check

Open the mixed-mode CUE with `--system 'Mega CD'`, using a locally configured
BIOS. Test title/options/credits, both areas, preloading, pause, returning to the
title during a preload and the missing-area recovery check. Watch for old text,
stale sprites, CRAM dots, a map appearing before black and loss of input/music.
Options persist; local lantern progress resets when a scene is successfully left.

The tour covers nine successful scene changes, cancellation/drain, re-entry and
missing-area rollback. `AUTO PASS` is not a listening test. Repeat the manual
flow in PAL for presentation/music timing. Real consoles, BIOS/model differences,
disc retries/seeks, stack high-water, electrical ownership timing and actual
audio quality still need hardware qualification. Dynamic code unload, 1M-mode
streaming and ASIC coexistence are outside this Lab.

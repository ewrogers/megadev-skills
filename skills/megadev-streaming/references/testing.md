# Verification and failure evidence

Run a fresh installed generator, then build it with `ewrogers/megadev:latest`
on ARM64 and AMD64. Compare compiled modules, disc files and boot payloads,
not complete ISO timestamps. Keep the build/reader/target revisions in reports.
The repository runs US, JP and EU release, US debug and US automatic-tour cases.
Docker test containers have a two-CPU quota; architecture jobs run independently.

Exercise the actual generated target code, not a second implementation that
only agrees with itself:

- Directory lookup and ABI preservation; missing and oversized records refused
  before any disc write. Check exact fit and last-sector padding boundaries.
- Delayed/failed reads, duplicate frame observations, retirement and late results.
- Cancellation during a live read, timeout while the drive still owns its buffer,
  and a grant that never confirms. No second request or speculative RAM access.
- Corrupt headers/palettes/map indices/checksums, bounded validation, partial
  uploads and READY becoming true only after the last acknowledged chunk.
- Active-display and late-blank callbacks issue no CRAM commands; early blank
  updates the changed line once and skips unchanged colors. Confirm fades visually
  in an emulator and on hardware; a modeled status bit does not measure timing.
- Queued frame bytes remain unchanged when Word RAM changes; VBlank never reads
  that region, and a consumed frame is not replayed.

Portable C fixtures test size arithmetic and incremental validation at O0/O3,
with undefined-behavior checking where supported. The paired original-68000
execution harness runs the real adapters, pinned `find_file` and C ABI shim;
its synthetic sector producer supplies delayed/failing data. It models ownership
and mailbox visibility, **not the drive/CDC/BIOS state machine or bus timing**.
Ares exercises the real reader/BIOS/VDP path. It still cannot establish real
seek/retry deadlines, console/model compatibility or electrical timing.

In ares, boot with `--system 'Mega CD'`. Move while the next area loads, try the
slow upload option, cancel/reload, request a missing file, then warp repeatedly.
Check artwork, HUD stability and continued controls. Repeat PAL/NTSC. The tour
includes a load, preload, warp, cancelled read and missing-file recovery; PASS
is a diagnostic, not a substitute for manual visual/input checks.

Physical checks still needed: cold/warm drive seeks, scratched/missing media,
1x throughput under load, multiple console/BIOS models, stack headroom, and any
concurrent CD-DA, PCM, ASIC or DMA service. Do not claim those passed from an
instruction emulator or ares screenshot.

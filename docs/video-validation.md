# Video validation

Cinema Lab's original Lantern Flight is a native 160×96 tile animation, not a
photographic FMV decoder. It streams 48 full 4bpp frames from 12 sector-aligned
CIN1 files. Nominal presentation is 10 images/sec; actual page-flip counts and
late-frame holds are displayed. The Lab never equates 60 Hz refresh with 60
new images/sec. Its art, music and controls are illustrative.

## Contracts and regressions

Portable tests compare exact exported native frame bytes, header fields, sums,
palette legality, frame uniqueness, and PCM sentinel safety. Runtime tests check
bounded upload lengths, ordered presentation, complete-frame deadlines, pause,
late holds and final-frame completion at O0/O3. PAL and NTSC use the same 300 Hz
presentation timeline.

Original-68000 tests execute both compiled processors: Sub file lookup, direct
CDC completion, two PRG cache groups, bounded Word RAM copies, checksums and
Main's token/ownership/copy/retirement. All 48 frames must match disc payloads,
and only 12 group reads may occur. Mutated headers, pixel sums, missing files and
short files fail. A timeout does not clear a still-live request.

VDP tests execute initial table setup, capture exact Main RAM DMA bytes, reject
invalid upload ranges, defer late/active-display packets and check final flips.
The first emulator prototype exposed a table alias: default Plane B occupied
E000, also used by the second Plane A page. Plane B now uses A000, separate from
both image pages. This is tested through actual register writes, not source text.

The loading pipeline initially lacked sufficient read-ahead. It now prefetches
into the other fixed Sub PRG group while the current one remains available,
without changing the Main frame reservation. Normal packets allow 4 KiB uploads;
256-byte mode deliberately demonstrates slower complete-frame presentation.

## Matrix and limits

US/JP/EU release, US debug and US automatic tour are built for ARM64 and AMD64.
Each container is limited to two CPUs and uses `ewrogers/megadev:latest`.
Local ARM64 execution is native; local AMD64 uses Docker emulation. CI uses
native architecture runners and compares complete target/disc payload matrices,
requiring paired execution evidence and all frame groups.

The inspected image uses Megadev 7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef,
GCC 16.2.0 and original-68000 defaults. Tests model CDC completion, Word RAM
arbitration, DMA and PCM registers. They do not measure physical seek latency,
VDP transfer duration, audio quality or actual pixel output. Emulator acceptance
and final CI results are recorded in the PR; physical hardware is unqualified.

A general footage encoder, changed-tile codec and synchronized long-voice track
would require further bounds, malformed-stream tests and hardware measurements.
The resident PCM phrase/cues demonstrate drive separation and presentation-event
scheduling, not sample-accurate lip sync or uninterrupted long audio streaming.

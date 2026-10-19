# Compression validation

The compression slice adds `megadev-compression` and a standalone, intentionally
silent Compression Lab. It compares RAW and Kosinski representations of the
same prepared artwork, including an intentionally incompressible noise sample.
The art and controls are illustrative, not a required game design.

## Audited implementation

The source audit used Megadev revision
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` from
`ewrogers/megadev:latest` (GCC 16.2.0). The Lab calls the real `Kos_Decomp`
assembly through a C ABI shim. The SDK header's inline wrapper has an operand
and clobber mismatch, so it is not presented as a verified interface.

A bounded C parser validates every source token, descriptor reload, output size
and history reference before the unbounded assembly decoder runs. Input remains
immutable and Sub-owned throughout. Main checks the decoded header, palette,
map and CRC before preparing resident upload snapshots for VBlank. Cancellation
discards only after the actual read/decode retires. This composes with streaming;
it does not add another drive or Word RAM owner.

## Size and RAM observations

| Sample | RAW bytes / sectors | KOS bytes / sectors | Decoded bytes | Active buffers RAW / KOS |
| --- | ---: | ---: | ---: | ---: |
| Garden | 15,680 / 8 | 7,256 / 4 | 15,680 | 32,064 / 23,872 |
| Cavern | 21,216 / 11 | 10,030 / 5 | 21,216 | 43,744 / 31,456 |
| Random noise | 15,680 / 8 | 16,972 / 9 | 15,680 | 32,064 / 34,112 |

Active buffers mean sector-rounded input plus decoded output. The Lab reserves
two separate 32 KiB Word RAM ranges, or 64 KiB total, regardless of the active
byte count. Other resident state and VRAM are separate. RAW uses the same
staging/copy path; a direct raw read to its final destination could avoid that
extra input buffer. Both representations are present on this comparison disc,
so these figures do not claim the entire disc shrinks.

Read duration counts Sub VBlank intervals. Decode/copy duration reads the shared
12-bit stopwatch without resetting it: 30.72 microseconds per tick. Main supplies
an independent VBlank epoch to distinguish wraps during longer Sub jobs; only a
unique candidate within conservative PAL/NTSC frequency bounds and 32 epochs is
accepted. Ambiguous/inconsistent measurements display RANGE. Validation and
publication time are not included in the decode figure.

## Automated evidence

- Host tests exercise O0/O3 validation with UBSan, truncated streams, invalid
  history/lengths, stopwatch wrap bounds, sector budgets and deterministic assets.
- Original-68000 execution runs the actual SDK decoder, byte-tail raw copy,
  short/long/continuation tokens, overlap and maximum-distance references.
  Output guards, preserved registers and stack restoration are checked.
- Paired Main/Sub execution verifies all six assets, every decoded/uploaded byte,
  size preflight, source/output ownership, missing/oversized/malformed input,
  cancellation, deadline retention, late completion and successful retry.
  VBlank is tested with Word RAM unavailable to Main to prove it uses snapshots.
- Fresh ARM64 and AMD64 builds pass US/JP/EU release, US debug and the automatic
  tour. Target payloads and executed-check reports match across architectures.
- Installed generation is tested with telemetry disabled. README and skills.sh
  include the skill in Systems. CI repeats the two native architecture matrices.

The simulated stopwatch checks wiring and arithmetic, not instruction timing.
The automatic tour covers all six sample/format combinations, consumes a preload
without rereading and loads behind a fade. Emulator observations and manual
checks are recorded in the PR; they are separate from byte-level proof.

## Measurement limits

Ares is useful for display, handoffs and regional checks, not proof of optical
seek latency or console cycle cost. Physical hardware still needs cold/warm
seek, media/BIOS, stopwatch, bus contention and simultaneous audio/ASIC/upload
qualification. Do not derive a universal codec ranking from these sample files.
See the skill's measurement reference for the repeatable procedure.

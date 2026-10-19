# Measurement and validation

## Measure real stages

The Lab records Sub VBlank intervals from read submission to the completed CDC
operation, then separately times the actual SDK decoder (or raw copy). Directory
lookup, preflight, Main validation and VRAM publication are not included in that
decode figure. File sectors are occupied payload sectors, not the complete ISO
size and not a measurement of seek overhead.

The audited [Sub stopwatch definition](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.def.h)
is a **12-bit counter with 30.72 microseconds per tick**, wrapping every
125.82912 ms. Read start/end snapshots; do not reset it, because the CDC uses
this shared clock. Main publishes its independent VBlank epoch in COMCMD3,
including while Sub is decoding. The Lab uses that epoch to bound the elapsed
interval and unwrap the stopwatch only when exactly one candidate fits.
The bounds allow 49–51 Hz PAL or 59–61 Hz NTSC and at most 32 crossed
boundaries; longer, ambiguous or inconsistent intervals display RANGE.
This depends on the Main VBlank service remaining timely: Sub interrupt counts
can coalesce during decoding and cannot replace this independent epoch.

Interrupt service and bus contention can affect elapsed decode time. Small
values also include timestamp/call overhead. Repeat runs with the same source,
region, image and instrumentation; distinguish cold and warm seeks. A read
frame is 1/50 or 1/60 second, not a universal millisecond. Do not call these
figures CPU utilization, instruction counts or hardware guarantees.

## Executable checks

Portable C checks run the bounded validator at O0/O3 with UBSan where supported.
They cover descriptor boundaries, overlapping references, truncated sources,
wrong output sizes, references before output history and stopwatch wrap/range
rejection. The generator checks sector-rounded capacity and produces a case
where compression grows the file. Generated output is deterministic and repaired
if damaged; existing projects are never overwritten by the creator.

Original-68000 execution tests run the **actual Megadev Kosinski assembly**, not
only an authoring decoder. They compare decoded bytes against the original for
art, noise, literal/long-reference streams, overlapping runs and short/continue
tokens. Destination guards, C callee-saved registers and stack restoration are checked.
The raw baseline uses an aligned longword assembly copy with a tested byte tail,
so it is not artificially penalized by a naive C byte-copy loop.

Paired adapter execution models shared Word RAM, CD completion and stopwatch
reads. It verifies all raw/packed samples, output bytes, metadata, Main's map
copy and every uploaded pattern byte. Cases include an overlapping request,
cancellation during reading, oversize/missing files, bad result/size/format,
a deadline with retained ownership, late retirement and a successful retry.
The modeled 37-tick interval checks plumbing, **not** the decoder's cycle cost.

Run the installed generator with telemetry disabled in CI. Build fresh projects
for ARM64/AMD64 US/JP/EU release, US debug and automatic tour; compare module,
boot and disc-file payloads. Missing executed compression evidence or sample
files must fail report comparison. ISO timestamps alone are not useful hashes.

## In ares

Open the CUE as **Mega CD** with your own BIOS. Check:

- The same garden/cavern as RAW and KOS look identical and complete.
- Noise is labeled intentional and remains the same through either format.
- Compare reports smaller sectors for art but larger sectors for packed noise.
- Load fades out, keeps the foreground counter serviced, then fades in.
- Preload leaves the old picture intact; A SHOW PRELOAD switches without a new
  read. C drops/cancels without showing partially prepared content.
- READ/DEC return values, the lantern keeps moving and no CRAM dots or stale
  glyphs appear. Start pauses the lantern without suspending loading service.
- Repeat PAL and NTSC. Byte results match; read frame counts and scheduling
  opportunities can differ. Decoder time must not be scaled by video rate.

The automatic tour exercises the six valid combinations, consumes a preload
without rereading it, and loads behind a fade. Manual transition and
presentation checks remain separate from byte-level proof.

## Still needs physical hardware

Qualify cold/warm seeks, actual media/BIOS/drive behavior, register timing,
shared-bus contention and the game's worst simultaneous audio/ASIC/VRAM workload.
Unicorn is a behavioral CPU oracle, not a cycle-accurate Sega CD. Ares numbers
are emulator observations. Neither establishes a universal codec speed ranking.

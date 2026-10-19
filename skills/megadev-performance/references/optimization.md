# Optimize the measured bottleneck

Start with correct release code and a representative workload. Keep a reference
or an independent oracle. Read the generated original-68000 assembly when source
changes are surprising; fewer C lines or fewer emulated instructions do not
necessarily mean fewer cycles. Do not defeat the toolchain's instruction/aliasing
safeguards or blame the compiler before ruling out undefined behavior and bad ABIs.

## Main CPU work

Move invariant work out of hot loops, bound indices/counts and use incremental
updates when their range assumptions are proven. The Lab compares repeated modulo
300 with advancing a cursor by 13 and one conditional subtraction. Its validated
phase/count bounds make both equivalent. This is not a universal replacement for
modulo with arbitrary strides or integer overflow. The inspected release reference
contains DIVU; the incremental loop avoids it. Actual benefit still needs measurement.

Use native fixed-point widths and data layouts that fit the actual game. Tables,
precomputed transforms or resident snapshots can trade RAM for CPU time, but record
their lifetimes and cache/rebuild costs. Do not add broad `volatile`, inline assembly,
a global pool or a new allocator just to make a benchmark number improve.

## Sub, ASIC and transfers

Measure command/grant wait, useful work, completion observation, return/copy and
publication as separate boundaries when needed. A late Main poll or a leased
upload buffer can throttle visible ASIC output even after the hardware finishes.
Overlap only when independent buffers and ownership make it safe. Keep source art
resident and avoid rebuilding invariant trace/source data unnecessarily.

The Lab reuses the scaling skill's snapshot pipeline and two-chunk early-blank
upload. It does not treat ASIC completion as permission to replace a snapshot still
leased by DMA. Batch transfers and update changed HUD/map data; budget table,
palette and sprite-list traffic in addition to pattern payloads. Defer late work
instead of scribbling CRAM or VRAM during an unsafe phase.

For a real game, include CD data loads, PCM/CD-DA and other bus/BIOS responsibilities
in a combined workload. Preserve their service cadence and failure/cancel paths.
A busy Sub CPU is not an unlimited worker thread, and an ASIC job is not free VDP
bandwidth. This Lab's combined case is **Main CPU + ASIC + VRAM**, not a CD/audio
contention benchmark. Use the audio/streaming contracts and their regression suites
when extending it; no maximum sustained audio/drive claim is made here.

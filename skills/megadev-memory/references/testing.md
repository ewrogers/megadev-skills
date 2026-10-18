# Tests and qualification

## Host contracts

Run the repository tests, or compile the installed Lab's `memory.c` with your own C17 fixtures. The host suite checks exact fits through 128 KiB, alignment/padding, one-byte/full/overflow refusal without mutation, generation invalidation, pool identity, leased reset refusal, release misuse, view pinning and generation exhaustion. Pattern tests cover chunk bounds, wrong tokens, corruption and guarded output. O0/O3 builds use undefined-behavior checking where supported.

Layout tests use fabricated symbol maps with exact fits, overlaps, wrong physical capacities, one-byte linked overflow, oversized embedded buffers and a payload exceeding one 1M bank. An embedded arena is reported but not added twice.

## Executed target code

`tests/integration.py --skill megadev-memory` builds US/JP/EU release, US debug and US tour. It checks startup, runs the actual compiled Main/Sub adapters on original-68000 CPU models and compares target payloads on ARM64/AMD64. Native CI uses separate runners; local Docker may emulate the other host architecture.

The paired harness models one physical 256 KiB array with 1M word interleaving and complementary CPU assignments. It refuses the Main cell-mapped alias as a second bank and rejects accesses through the wrong Sub view or unowned 2M region. Tests exercise both RET directions, prepare/read overlap on different banks, entry/exit with different prior ownership, repeated/skipped ticks, reader-blocked swaps, lifetime reset and a failed grant that cannot authorize reuse.

This models memory/register visibility, not electrical timing, arbitrary instruction-level concurrency, CDC/ASIC operations or a complete dual-CPU machine. The actual linker report is attached to each target case, with reservations and embedded backing validated against symbols.

## Ares and hardware

Launch with `ares --system "Mega CD" memory_lab.cue` and the user's configured legal BIOS. `TOUR=1` should reach AUTO PASS after nine bank operations plus the lifetime test, with zero errors and clocks continuing. Keep the disc immutable while loaded.

In the manual build, Left/Right or Start cycles Budgets, Lifetimes and Word RAM. On Lifetimes, A advances the guided checks, B resets when unpinned and C releases the simulated deferred user early. On Word RAM, A prepares, B enters/exits 1M and C exchanges a ready bank. Try repeated swaps and prepare again before another exchange.

Compile PAL separately and distinguish that from an actual PAL emulator/console run. Real BIOS/model variation, mode/ownership timing, interrupt deadlines, hardware DMA quirks and simultaneous disc/audio/ASIC use remain physical/integration qualification. This diagnostic is not a measured game-performance benchmark or a demonstration of background disc streaming.

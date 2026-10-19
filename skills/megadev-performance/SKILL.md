---
name: megadev-performance
description: Measure and improve Megadev Sega CD/Mega CD workloads with bounded stopwatch samples, Main/Sub pipeline observations, upload accounting and controlled PAL/NTSC comparisons. Use for profiling and optimization; keep correctness regressions and existing runtime service owners intact, and distinguish elapsed time from CPU utilization.
---

# Measure Megadev performance

Start with a repeatable workload, a correctness check and one suspected bottleneck. Preserve the exact build, region/video target, assets and input configuration. Use `ewrogers/megadev:latest`, retain `doctor` output and inspect the selected SDK sources before teaching a timing API. Read [Megadev timing integration](references/megadev-integration.md) for verified facilities and shared-resource constraints.

## Say what a measurement means

Use [measurement contracts](references/measurement.md) to distinguish a completed batch, elapsed stopwatch ticks, serviced VBlanks, observed request latency, completed uploads and CPU utilization. None is interchangeable. A busy pipeline or a long wait is not proof that a CPU is executing useful work.

Keep interrupt/CD/audio service enabled unless an isolated measurement specifically requires otherwise. Report interruptions and sampling overhead. Do not reset a shared stopwatch, install a second frame/transfer owner, silently drop long samples or call emulator instruction counts hardware cycles.

## Compare one change at a time

Read [controlled experiments](references/experiments.md) for warm-up, fixed inputs, observer-cost checks and region comparisons. Verify equivalent output before accepting an optimization. Check several trials, worst observed cases and the release build. Preserve a slow reference/oracle for tests; do not make production execute it inside every timed batch.

[Performance Lab](assets/performance-lab/README.md) compares remainder versus incremental wrapping, resident VRAM uploads, the real graphics ASIC pipeline and combined Main/ASIC/VRAM work. It provides HUD/timer toggles and drains leases before changing cases. Its live counters are not a console-wide CPU meter or a maximum-bandwidth certification.

Use the [comparison tool](scripts/compare_runs.py) to normalize completed run counters over their actual reported window. It rejects failed/incomplete runs and changes along more than one declared configuration axis. It processes recorded numbers; it does not capture ares or prove where those numbers came from.

## Improve the actual bottleneck

Use [optimization guidance](references/optimization.md) for bounded work, hot-loop arithmetic, batching, residency, overlap and contention. Reuse system/memory ownership, streaming/audio scheduling and graphics publication; performance work does not authorize replacing their contracts. Recheck output, input responsiveness, cancellation, late results and PAL/NTSC after changes.

Follow [qualification](references/testing.md) to separate host equivalence, original-68000 execution, native ARM64/AMD64 builds, ares and physical-console evidence. `megadev-testing` owns reproducible regressions; `megadev-debug` explains failures; `megadev-scaling` owns ASIC layouts/handoffs. This skill measures and tunes those systems. Lab visuals, controls and workloads illustrate experiments rather than prescribing game design or performance targets.

---
name: megadev-debug
description: Diagnose Megadev Sega CD/Mega CD failures using CPU/module-aware symbols, bounded event traces, first-fault snapshots and reproducible fault injection. Use for crashes, stalls, stale replies, bad buffers and low-overhead instrumentation; keep runtime coordination, memory policy and graphics ownership in their existing systems.
---

# Debug Megadev games

Start with the symptom, reproduction steps, region, selected image revision and exact build artifacts. Establish whether Main, Sub or both stopped making progress. Do not replace the game's service loops or infer a crash merely because a buffer is draining.

Read [Megadev integration](references/megadev-integration.md) before assuming a helper, debugger connection or exception frame. Inspect the selected container source. The Lab's `dbg_*` and `sy_*` functions are project code, not Megadev APIs. Do not substitute SGDK debug/task APIs.

## Preserve evidence before trying recovery

Use [diagnostic workflow](references/workflow.md) to record the first failing transition and its request identity, phase, owner and result. Sample a committed reply before the fresh ownership check. Distinguish a deadline, cancellation, rejected request, corrupt returned data and an unrecoverable ownership stall.

Use fixed, bounded traces and one writer per CPU. Log transitions rather than every polling iteration. Keep Main/Sub clocks separate unless the game explicitly correlates them. Freeze useful evidence without stopping normal cleanup; never return a busy buffer to an allocator just because its deadline expired.

Keep the existing renderer, interrupt, input, CD and audio owners. No active-display CRAM writes, interrupt-time text formatting, unbounded logging, or direct foreign-CPU pointers. Compare instrumented and release behavior; a diagnostic that consumes the frame can create the symptom it reports.

## Resolve addresses in the right address space

Read [symbols and exceptions](references/symbols-and-exceptions.md) for exact-build ELF lookup, module residency, source-line limits and fatal-handler caveats. Save the disc, ELF files, `.sym` files, compiler/SDK identity and build flags together. Main/Sub can use the same numeric address for different memory. A module's code must still be resident before a captured address can identify it.

`DEBUG=1` defines a macro; it does not automatically enable debug information or install exception handlers. Keep a reproducible release case, then make a separate `-g` build if source lines are needed. Do not promise that ares exposes a GDB server simply because the toolchain includes GDB.

## Reproduce and qualify the fix

[Debug Lab](assets/debug-lab/README.md) adds transition recording and fault snapshots to the existing coordination fixture. Use [qualification](references/testing.md) for host bounds tests, paired original-68000 execution, real ELF lookups, ARM64/AMD64 comparison and ares checks. Report what remains untested on physical hardware.

Scope boundaries: `megadev-system` owns request/cancel/drain semantics; `megadev-memory` owns allocation and 1M/2M lifetimes; `megadev-project` owns bootstrap and module loading; graphics/audio/streaming skills own those services. This skill adds evidence and diagnosis, not a competing runtime. Lab panels, controls and fault profiles are illustrative and must not prescribe the user's game design.

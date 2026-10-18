---
name: megadev-system
description: Coordinate Main/Sub CPU work in Megadev Sega CD/Mega CD games with committed commands, bounded jobs, safe Word RAM handoffs, cancellation and timeout recovery. Use for inter-CPU request lifetimes and platform-service integration, rather than asset conversion, memory allocation or game-specific threading.
---

# Megadev system coordination

Inspect the selected toolchain and the game's current communication, interrupt and Word RAM owners. Use [verified integration](references/megadev-integration.md) for actual Megadev definitions and BIOS boundaries. The Lab's `sy_*` functions are project code, not SDK APIs. Do not substitute SGDK tasks, joypad or streaming functions.

## Give each request a complete lifetime

Read [command protocol](references/command-protocol.md) when designing or changing the wire contract. Assign one writer to each mailbox field, publish metadata before its commit word, and hold it stable until the receiver retires the request. An aligned word commit does not make a multiword header atomic.

Distinguish request identity, command kind, progress and completion. Match replies to the current request; a nonzero status or a reused opcode alone is not enough. Use fixed buffer descriptors or validated bounded offsets/counts instead of passing a Main pointer for the Sub CPU to dereference.

## Keep ownership separate from completion

Read [handoffs and recovery](references/recovery.md) before adding cancellation, deadlines or buffer reuse. A reply does not substitute for checking Word RAM mode/ownership. A timeout does not prove that the worker stopped or returned its memory.

Keep abandoned work isolated until it returns safely. Discard its output, retire both sides of the handshake, then admit another request. If the peer never returns, keep that resource unavailable and let the game continue or offer an explicit recovery path appropriate to its design. Do not silently reset the Sub CPU, force an ownership change or restart disc/audio services to recover a menu action.

## Preserve platform service

Use bounded foreground work slices and keep the Sub BIOS/CD access service running. Two CPUs are not an unlimited background thread pool. The Lab's one job slot and one chunk per display tick are illustrative policies, not required game architecture.

Use an elapsed-time clock that keeps advancing when UI or rendering work is slower. Preserve the game's existing frame, input, audio and transfer owners; integrating a command service does not authorize replacing them. The standalone Lab owns these facilities only within its own disc.

## Prove the whole exchange

[System Lab](assets/system-lab/README.md) demonstrates a responsive Main-side control light, Sub-side buffer transformation, rejected requests, cancellation and a deliberately late result. Follow [tests and qualification](references/testing.md) for host failure cases, paired compiled-68000 execution, architecture comparisons and ares checks.

Keep the scope distinct: `megadev-project` handles bootstrap/module startup; `megadev-hardware` explains registers and memory views; `megadev-input` owns device protocols; `megadev-graphics` owns rendering. This skill connects existing services through request/ownership contracts. Detailed allocation, 1M bank scheduling, CD streaming, audio policy and ASIC jobs belong to their respective systems.

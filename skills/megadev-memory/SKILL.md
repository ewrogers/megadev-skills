---
name: megadev-memory
description: Budget and place RAM in Megadev Sega CD/Mega CD projects, verify linked module reservations, manage transient allocation lifetimes and safely use 1M/2M Word RAM views. Use for memory placement, buffer leases and bank/view validity rather than general command coordination or disc streaming.
---

# Megadev memory

Inspect the selected toolchain, linker layouts and existing resource owners before adding a pool or changing a memory mode. Main Work RAM, Sub Program RAM, Word RAM, PCM RAM, Z80 RAM and VDP memory are separate resources. Do not combine their capacities into a shared C heap.

## Account for the linked program

Read [pools and placement](references/pools-and-layout.md) for CPU views, BIOS/stack reservations and linked versus reserved bytes. The standalone Lab's `tools/report_layout.py` checks its explicit `layout.json` against actual Megadev symbol files. Adapt reservations to the game; the Lab's large code windows and growth margins are examples, not mandatory overhead.

Count code, constants, BSS, initialized data, startup/access services, stacks and resident pools. An arena array inside BSS is already counted there. Unassigned address space is not automatically an initialized allocator. A stack reservation does not prove its high-water usage.

## Choose a lifetime

Read [allocation lifetimes](references/lifetimes.md) for resident, scene and scratch storage, alignment, generation checks and leases. The included `ma_*` and `mv_*` functions are project-owned examples, not Megadev allocator APIs.

Use a storage policy that fits the game. A scene arena can reset in bulk once its users finish; stable pools or individual allocations may suit different lifetimes. Refuse an allocation that does not fit without silently resizing buffers or moving live objects. An unload must wait for queued transfers/workers that still reference the backing bytes.

## Treat Word RAM as an owned view

Read [Word RAM modes](references/word-ram.md) before changing MODE, DMNA or RET. In 2M mode one CPU owns all 256 KiB. In 1M mode each owns a different 128 KiB bank; swapping changes what the same linear pointer addresses.

Invalidate old views before transitions, stop all readers/writers/DMA/ASIC use, request the handoff and check the resulting mode/bank. Do not infer ownership from a command reply alone. Do not execute code or leave stacks in a Word RAM region while remapping it. On failure, keep the resource isolated instead of guessing that an old pointer is safe.

## Verify the placement and handoff

[Memory Lab](assets/memory-lab/README.md) shows linked RAM budgets, a refused unload while pinned, stale-handle rejection and alternating 1M banks with explicit return to 2M. Follow [tests and evidence](references/testing.md) for host bounds/failure tests, compiled dual-68000 execution, architecture comparison and ares checks.

Keep responsibilities distinct: `megadev-planning` estimates budgets/lifetimes before implementation; this skill checks concrete placement and runtime validity. `megadev-system` owns command/acknowledgment/deadline contracts, `megadev-project` owns startup, and `megadev-graphics` owns VDP transfers. Disc read scheduling and 1M CDC/ASIC integration are separate streaming/scaling work, not proven by this Lab.

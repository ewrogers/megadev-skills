---
name: megadev-gameplay
description: Implement or review real-time gameplay in Megadev Sega CD/Mega CD projects with fixed-step timing, movement, collision, camera bounds, object lifetimes and responsive play during background work. Use for the simulation loop and its integration, rather than device protocols, rendering APIs or a particular genre's rules.
---

# Megadev gameplay

Start with the game's intended movement and rules, its existing clock/input/renderer owners, and the actual toolchain headers. Megadev has no SGDK sprite engine or general physics API. The Lab's `gp_*` calls are application code, not SDK functions.

## Choose the clock before tuning movement

Use [timing and input](references/timing-and-input.md) for fixed-step accumulation, PAL/NTSC, edge consumption, pause and overload policies. Tune speeds in units per second; do not make one display frame equal one gameplay tick on both video standards. Reuse an existing simulation clock instead of adding another.

Keep a bounded amount of work per foreground iteration. Decide explicitly what happens after a long stall. Service deadlines and loading/audio handshakes must continue while gameplay is paused. A fixed-step model does not prove the full game meets its frame budget.

## Separate simulation from its picture

Use [movement and lifetimes](references/movement-and-lifetimes.md) for coordinate ranges, collision boxes, axis sliding, camera bounds, pools and area state. Let collision and pickups depend on world state, not sprite visibility or successful rendering. Keep collision bounds independent of artwork dimensions.

Choose collision techniques for the actual maximum movement: small bounded substeps work for slow actors, while fast shots need swept tests. Avoid presenting the Lab's simple tile test as a general collision engine.

## Keep background work optional to each step

Use [Megadev integration](references/megadev-integration.md) for verified BIOS fields, frame publication, Main/Sub ownership and memory budgets. Authoritative movement stays on Main in the Lab; its optional Sub job exercises a separate service lifetime. Never read live Word RAM from the renderer while Sub owns it, or block each movement tick on a job reply.

Use the existing owners for input, VDP, disc, audio and resource retirement. `megadev-system` owns command protocols; `megadev-streaming` owns loading; `megadev-memory` owns allocation and bank scheduling; `megadev-graphics` owns rendering; `megadev-scenes` owns transitions. This skill connects those contracts to gameplay rather than replacing them.

## Validate behavior and qualify the result

[Adventure Lab](assets/adventure-lab/README.md) is a standalone two-area example with movement, solid obstacles, collectibles, patrol enemies, a cooldown pulse, respawns and a bounded cosmetic pool. Its art, sound, controls and rules are illustrative; adapt the smallest useful pattern to the user's game.

Use [validation](references/validation.md) for executable model checks, paired compiled-68000 tests and ares PAL/NTSC review. Measure the actual combined workload before increasing actor counts. Treat emulator success, instruction execution and physical hardware qualification as different evidence.

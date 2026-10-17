# From idea to proof

## Keep the game brief separate

Start with the desired experience and existing code. A quiet exploration game and a crowded action game may share a loader while needing different latency, input and presentation choices. No Lab's art, music, buttons, upgrade tracks or progression belong in the plan unless the user asks for them.

Keep four kinds of statement distinct:

- **Hardware fact:** Word RAM is 256 KiB.
- **Framework/configuration fact:** the selected bootstrap reserves particular BIOS and module regions.
- **Project choice:** keep the player and UI resident across room changes.
- **Evidence or assumption:** the current compressed room is 96 KiB; seek time has not been measured.

Use real file sizes, linker maps and measured timings as they become available. Estimates let work begin; they are not facts to repeat later without checking.

## Pick lifetimes before pool sizes

List what remains live during play, loading, menus and transitions. Count both old and new assets if they overlap. The packed file, decoded output, decompressor scratch, active module, stacks and pending upload buffers can all exist together.

Draw the handoff in plain language. For example: resident Main code stops using the room, grants 2M Word RAM, Sub loads it, Sub returns ownership, Main initializes and calls the new module. This is one valid arrangement, not a universal architecture. Streaming in 1M mode needs a different buffer schedule and a smaller per-CPU bank.

Count a shared resident allocation once in each phase where it is live. Do not sum mutually exclusive rooms into one peak or forget the transition in which both coexist. Explain aliases and reuse; the budget helper cannot prove that two names refer to one physical allocation.

Keep code responsible for interrupts and resource ownership resident. A module's data/BSS initialization, imports and stack usage are part of feasibility, not details to postpone until after art is complete.

## Make waiting intentional

For each content request, identify when it begins, what can run while it waits and when the content is actually usable. Reading from CD, decoding and uploading to VRAM are different steps.

A useful load plan includes a late path: retain the old room, hold a transition, delay an optional effect, or otherwise preserve valid state. Pick the behavior that fits the user's game. A fade can hide presentation changes, but it does not make an unsafe memory handoff safe.

Keep audio in this schedule. Specify whether disc music stops before a data read, what fills the gap and when playback resumes. Do not substitute a cartridge driver's API for a Megadev sound design.

## Choose the next proof

Prefer one small executable experiment for the largest unknown:

| Risk | Useful proof | Evidence to keep |
| --- | --- | --- |
| Main memory pressure | Link the resident/module layout and reload repeatedly | Map, stack allowance/high-water check, startup results |
| Tight 1M banks | Stage both buffers with decoded worst-case sizes | Per-bank occupancy and ownership trace |
| Load misses its window | Read representative content while the foreground runs | Seek/read/decode/upload timing, late-load fallback |
| Audio during loading | Exercise the actual chosen music/sample path | Audible result and service/refill behavior |
| Crowded rendering | Show the expected worst overlap and upload load | Frame cost, scanline pressure and visual degradation policy |
| Disc/BIOS variation | Repeat boot/load/error scenarios beyond one emulator | Exact region, BIOS/model, medium and hardware observations |

Use the project's existing tests and clocks rather than adding rival systems. Cross-build with native ARM64 and AMD64 toolchains when implementation is introduced; compare relevant target payloads. A Python planning calculation does not need a new ROM merely to look more substantial.

## Deliver a plan people can use

Use the worksheet sections that matter. Lead with the proposed next milestone and the main trade-off. Put technical tables and assumptions behind that explanation. A user should know what to build next without first mastering every register.

The planning skill owns feasibility, lifetimes and proof criteria. The project skill owns boot/build scaffolding and startup implementation. Future graphics, audio, streaming, hardware, testing and release work should implement and measure the selected design rather than silently replace its ownership or resource policy.

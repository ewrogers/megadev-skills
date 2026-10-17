# Game plan

## Player experience

- User's concept, controls, visual/audio direction and target audience:
- Smallest playable slice and what it must prove:
- Target regions/video modes, BIOS/model coverage and controller setup:

## Machine responsibilities

| Work | CPU/device | Resident location | Evidence / unknowns |
| --- | --- | --- | --- |
| Frame loop, input and VDP submission | | | |
| Disc access and communication | | | |
| Audio playback and sample refill | | | |
| Optional graphics ASIC work | | | |

## Content and lifetimes

List what stays resident, changes by location and is temporary. Include executable modules, initialized data, BSS, stacks, BIOS reservations, compressed input, decoded output, VDP tables, patterns, PCM sample markers and queues.

| Allocation | Physical pool | Peak bytes | Active phases | Owner/access | Measured or estimated? |
| --- | --- | --- | --- | --- | --- |
| | | | | | |

Link the editable budget JSON and actual linker maps. A budget total does not prove address layout.

## Loading and handoffs

For each transition: source files, destination, packed/unpacked sizes, staging space, Word RAM mode/owner, module dependencies and when the old content becomes unused.

- Preload trigger and available time:
- Expected seek/read/decode/upload time, with evidence and uncertainty:
- If content is late or fails: what remains playable, visible and audible?
- Ownership handshake and quiescence before replacement:
- Timeout/cancellation policy and safe recovery:
- CD-DA playback versus data access policy, if applicable:

## Shared rendering and audio budgets

- VRAM layout, tile patterns versus tables, palette ownership:
- Worst crowded scanline: hardware sprites and sprite width:
- VBlank uploads, Main/Sub frame costs and bus waits to measure:
- PCM sample residency/refill, voice/channel priorities:
- Backup-RAM requirements and full/missing-media behavior, if saves are needed:

## Proof before expansion

| Risk | Small executable check | Pass criterion | Emulator / hardware evidence |
| --- | --- | --- | --- |
| | | | |

Record PAL and NTSC separately. Include slow/failed loads, repeated module changes, maximum visible load and audio during loading when relevant. State what still requires a physical drive or console.

## Decisions and next milestone

Record the chosen trade-off and rejected alternatives only where they explain a constraint. Keep aesthetic and genre choices with the user's brief, separate from hardware facts.

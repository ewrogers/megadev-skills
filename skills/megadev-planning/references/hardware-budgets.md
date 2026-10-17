# Hardware budgets

Use bytes/KiB for storage and microseconds or milliseconds for time. In hardware descriptions, **2M Word RAM means 2 megabits: 256 KiB**, not 2 MiB. A 1M bank is 128 KiB. A 68000 word is 16 bits, but the memory's name does not change its byte capacity.

## Different pools, different uses

| Physical pool | Capacity | Planning consequence |
| --- | --- | --- |
| Main Work RAM | 64 KiB | Resident Main code, game state, stacks, queues and BIOS reservations share it. |
| Sub Program RAM | 512 KiB | Sub services, stacks and staging share it; the standard Sub BIOS occupies the first 24 KiB. |
| Word RAM | 256 KiB | Shared storage with explicit 2M ownership or two 1M banks. Can hold code as well as data. |
| PCM wave RAM | 64 KiB | Dedicated sampled-audio storage, not general C heap space. Include terminators, loops and refill lifetimes. |
| VDP VRAM | 64 KiB | Patterns and VDP tables share it; add-on RAM does not enlarge it. |
| Z80 RAM | 8 KiB | Driver and any buffers must fit if the chosen audio design uses it. |

[Program design](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/program_design.md) describes the three CPU-side pools and ownership. Megadev's [Main BIOS notes](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/main_bios.md) identify workspace beginning at FFF700 and the system area at FFFD00. The example conservatively reserves the entire top 2304 bytes plus its own stack allowance; this is a planning choice, not a claim that every BIOS function uses that entire range. Reclaiming unused BIOS workspace requires auditing the actual services in use. Stacks grow toward lower addresses on the 68000.

The [ares Mega CD implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/mcd.cpp) corroborates Program/Word RAM allocation. Its [PCM implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/pcm.cpp) allocates 64 KiB of wave RAM. Treat emulator implementation as corroboration, not physical-console timing evidence. Megadev's [PCM declarations](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.h) expose 16 wave banks and eight channels; a small CPU-visible window is not the total sample capacity.

The CDC has dedicated buffering; do not add it to the game's freely allocatable RAM. Backup RAM is also separate. Ask the BIOS about usable save capacity rather than treating its raw physical size as available file space. Expansion backup cartridges and hybrid cartridge designs are separate requirements.

## Word RAM phases

In **2M mode**, one CPU owns all 256 KiB. Sub can fill it, then Main can consume it. If Main executes a module there, it must leave that module before granting the memory back. Keep critical callbacks and the handoff code resident elsewhere.

In **1M mode**, each CPU has a 128 KiB bank, and the banks can swap. This supports producer/consumer loading, but does not eliminate synchronization or guarantee enough decode/upload time. A 140 KiB Main allocation cannot fit its bank merely because Sub uses little of the other bank.

The graphics ASIC also uses Word RAM. Plan its source, destination, priority/format and ownership with the CPUs. It does not draw directly to the VDP or make arbitrary sprite art free to scale. Do not reserve a speculative ASIC pipeline for a game that does not need one.

## The original VDP still sets the visible limits

| Mode | Hardware sprites per frame | Sprites per scanline | Sprite pixels per scanline |
| --- | --- | --- | --- |
| H40 (320 pixels wide) | 80 | 20 | 320 |
| H32 (256 pixels wide) | 64 | 16 | 256 |

Count hardware sprite pieces, not actors. Include HUD sprites, effects and the worst horizontal overlap. Use full sprite width in the pixel demand estimate; transparent pixels do not make the sprite's width free. These limits and rendering details are explained by [Plutiedev's sprite guide](https://plutiedev.com/sprites).

An ordinary 8×8, 4-bpp pattern takes 32 bytes. Dividing VRAM by 32 gives 2048 pattern-sized slots before tables and other reservations, not 2048 freely usable art tiles. Tilemaps, sprite attributes and scrolling tables need their own room and valid alignment/address layouts.

There are four 16-entry palette lines. Each sprite or ordinary tile selects one line; transparency and the selected backdrop affect how index zero appears. Plan line ownership across UI, actors and scenery rather than forcing every asset into one global 15-color palette. Palette swaps are useful at transitions. Raster palette effects and shadow/highlight have additional rendering constraints and are not a default answer to every palette budget.

## Time, discs and audio

50 Hz gives a nominal 20 ms frame; 60 Hz gives about 16.67 ms. Budget Main and Sub separately. Do not add them and call the sum a frame cost, or assume both can access every resource concurrently. Measure VDP uploads, bus waits, interrupt work, audio service and the chosen ownership protocol.

A single-speed data transfer has a nominal Mode 1 payload rate of 75 × 2048 = 153600 bytes/s before software and access overhead. The [ares drive model](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/cdd.cpp) advances at 75 sectors/s; this is not a measured Megadev transfer benchmark. Seek time, file placement, read/decode/copy work and drive behavior make that an optimistic transfer component, not a guaranteed complete load time. Prefer contiguous packages for content used together when appropriate; avoid many small seeks.

CD-DA and data reads share the optical mechanism. Do not promise uninterrupted disc music while freely seeking for game data. Choose deliberate loading points, enough preloaded content or a RAM-resident sound path, then test the handoff. PCM sample storage/refill and the base console's FM/PSG resources have separate constraints.

Relevant Megadev source: [CD access](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/cdrom.md), [disc building](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/disc.md) and [module layout](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/modules.md). Inspect the selected `ewrogers/megadev:latest` source and record `doctor` before implementation; no planning helper can verify a future SDK revision or drive.

# Main CPU integration

## One input service

The base console's controller ports are on **Main**, not Sub. The normal Mega CD Main BIOS VBlank handler calls its own `BIOS_READ_JOYPAD`, as described in [Megadev's BIOS notes](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/main_bios.md). Calling an extended reader beside that handler is not a safe upgrade: extra TH edges can change six-button phases, and mouse transactions can consume motion twice.

For ordinary 3-button input, the existing BIOS hold/hit mirrors may be sufficient. For extended devices, use one integrated owner. The Lab installs a project-owned interrupt after the bootstrap has loaded the resident program. It does **not** keep BIOS controller polling enabled alongside the new backend. Consequently, `bios_joy*_hold/hit/type` are not the Lab's live input interface.

This is not an instruction to replace every game's interrupt handler. A renderer, raster effect, audio service or scene system may already own it. Integrate polling/snapshots into that design and retain every required frame service; never add a competing main loop or frame owner.

## What the Lab preserves

`input_irq` saves D0–D7/A0–A6, calls the documented `BIOS_TRIGGER_IFL2`, commits the prepared UI frame, restores registers and returns with RTE. The Sub interrupt/access-loop path remains active, and the HUD's SUB counter observes its heartbeat. The example uses direct Gate Array command/status registers, not BIOS communication mirrors.

It deliberately avoids `BIOS_COMM_SYNC` and its optional cached-command handshake. A game that relies on that service needs its own reviewed integration. This example is not a compatibility replacement for all BIOS behavior. Graphics updates and input service are now explicitly owned by the Lab.

The foreground uses an atomic STOP/unmask wait rather than BIOS's wait routine. That sample assumes supervisor-mode execution and only the configured interrupts; it is not a portable scheduler. Startup uses the existing project scaffold until the resident Main entry. One full interval passes before direct polling, allowing any prior BIOS six-button transaction to expire.

## I/O critical section and the Z80

`platform.c` uses Megadev `IO_DATA1/2`, `IO_CTRL1/2`, `Z80_REG_BUSREQ`, `Z80_BUS_REQUEST` and `Z80_BUS_RELEASE`. It is the Lab's sole foreground I/O owner. It masks Main interrupts during each polling batch and requests the Z80 bus when not already granted. A request that fails to grant within 512 status checks returns BUS BUSY samples; no controller pins are touched. A previously held grant is not released by this owner.

The adapter saves/restores the caller's interrupt mask on both success and failure, using audited supervisor-mode C ABI helpers. Call it from the normal foreground, not as another poll inside an interrupt. A register grant is not an ownership token across multiple competing subsystems. In an existing game, use its established Z80/bus policy; preserving register state does not authorize nested ownership.

Controller direction/data writes use byte-sized registers. Four NOPs follow writes. Serial acknowledgment waits have a per-nibble cap of 128 reads **and a whole-packet cap of 256**, then restore idle lines and discard incomplete data. These are bounded operation counts, not measured microsecond deadlines. Compiler/CPU costs and peripheral timing differ; tune them with hardware evidence. Interrupted/slow devices must not turn a game into an unbounded wait.

Source: Megadev [Z80 definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/z80.def.h), [BIOS service contracts](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h), Plutiedev [I/O ports](https://plutiedev.com/io-ports) and [controller guidance](https://plutiedev.com/controllers).

The Lab has no Z80 audio playback. Pausing the bus for input is not proof that FM/PCM drivers or CD audio remain unaffected. Measure the critical section and audio interaction before using it in a game.

## Displaying snapshots safely

The foreground updates state and prepares a private UI buffer. A published inactive frame contains at most 96 changed tile cells and eight mouse cursor sprites. The callback validates counts/offsets, writes those cells and updates the display cache only after committing them. Remaining UI changes are carried to later frames; input processing never waits for text to catch up.

Opaque glyph tiles keep the text steady over a fixed panel color. The callback does not poll ports, build strings, load files or wait for another frame. Do not use the inspector's redraw cadence as an input-event log: the game snapshot is authoritative.

Resident code/data uses FF0000–FF9FFF; mutable state uses FFA000–FFC3FF. The unused room compatibility fixture has FFC400–FFCBFF; Main's stack remains FFD000–FFDFFF and BIOS RAM begins at FFF700. These are sample reservations, not universal defaults. Prepared font patterns are at 0400–0FFF, the crosshair at 2000, and the existing BIOS VDP table addresses remain reserved. No per-frame pattern or palette uploads are required.

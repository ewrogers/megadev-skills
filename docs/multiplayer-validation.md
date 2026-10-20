# Multiplayer validation

## Candidate and scope

`megadev-multiplayer` adds a standalone Co-op Lab and reusable party policies.
The original procedural native-pixel assets, quiet PSG theme and garden rules
are illustrative. The candidate uses Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0 and binutils
2.47.20260726 from `ewrogers/megadev:latest`, pulled before validation. Container
builds use two CPUs, original 68000, no FPU/MMU, no auto-inc-dec and no strict
aliasing. Actual report files retain `doctor` output.

## Behavior exercised

- Four stable logical players across eight socket positions; nonsequential joins,
  full roster, holes, intentional leave and rejection of mouse snapshots.
- Held-button baselining; disconnect neutralization, five-second service grace,
  same-kind reconnect without healing and changed-kind expiration.
- Simultaneous pause requests combined once; join never also pauses. The camera
  refreshes on a roster change even while gameplay is paused.
- Independent damage immunity, three-second respawns, surviving teammates and
  cooldowns preserved, and recovery when all keepers are down.
- Bounded 25-position spawn search, valid fallback, solid tiles, world bounds and
  shared-camera framing under deterministic randomized play.
- Equal-time PAL/NTSC movement and timers, bounded catch-up, edge-only stun,
  clear stopping gameplay while its celebration clock and fanfare continue.
- Native asset regeneration, tile sizes and four valid Mega Drive palettes.

Host tests compile portable rules at O0/O3 with undefined-behavior sanitization;
6,000 seeded randomized roster updates independently assert identity uniqueness,
valid positions and visible camera bounds. They found a real paused-join framing
bug, fixed by updating the camera on lifecycle changes outside simulation steps.

## Target checks

The linked original-68000 contract executes the same behaviors, including complete-and-repeat autoplay. The demo uses an authored collision-safe circuit; it does not introduce a competing AI/pathfinding service. Qualification contracts run in the harness, not as a long blocking demo startup. Scripted MMIO
also executes the actual copied input readers: pad-six extension, mouse axes,
Team Player decoding, handshake timeout, EA mapping, bus timeout and restoring
the caller's interrupt mask. The custom IRQ preserves registers/exception frame
and emits one BIOS INT2 pulse without a second BIOS joypad read.

The actual frame publisher is immutable until consumed. Tests defer late VBlank,
compare exact SAT/HUD/scroll words, reject invalid index/count without VDP writes,
and check the 872-byte upload bound and absence of normal CRAM writes. Fanfare
port traces confirm changing notes and pause muting; this is not listening evidence.

Fresh container matrices cover US/JP NTSC release, EU PAL release, US debug,
CPU tour and explicit EA builds. Reports must contain executed multiplayer/input/
publication evidence in every case; comparison rejects missing cases/markers or
mismatched disc/module/boot bytes. The normal linker-overflow negative control
still rejects too-small Main RAM. Installation tests disable telemetry.

## Emulator and limits

The user confirmed NTSC movement, stun/recharge, pause and independent returns.
The initial clear state stopped its animation/audio clock and looked frozen;
the corrected build keeps a celebration clock, finite fanfare and moving
stars/coins with an explicit replay prompt. The user confirmed the revised celebration. Final PAL acceptance is pending review at the time of this note.

Ares scripted/bot runs are not physical multi-controller or multitap evidence.
Team Player/EA hardware, hot-plug electrical behavior, total frame-cycle margin,
real Z80/audio contention and physical-console presentation remain unqualified.
There is no new streaming or Sub job during this Lab's play; those ownership
contracts remain with existing skills rather than a fabricated per-player CPU.

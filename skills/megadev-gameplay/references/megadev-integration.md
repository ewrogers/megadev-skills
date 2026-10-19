# Verified Megadev integration

Verified against the pulled `ewrogers/megadev:latest` image: Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0. Public commands use
`latest`; record the selected revision/digest when validating another build.

## Actual API versus project code

- `lib/main/bios.h` exposes `bios_joy1_hold`, `bios_joy1_hit`, BIOS palette/VDP
  caches and callback fields; `lib/main/io.def.h` defines the pad masks.
- `lib/main/bios.def.h` names the BIOS service entry points, default Window
  A000, Plane A C000, Plane B E000, SAT B800 and HScroll BC00.
- `bridge.s` adapts BIOS calls to C while preserving the m68k ABI. Its project
  interrupt path calls `BIOS_TRIGGER_IFL2`, `BIOS_READ_JOYPAD`, then the bounded
  publisher, preserving interrupted registers. It avoids the BIOS communication
  cache handshake because the resident Main/Sub service owns its mailboxes.
- `lab_wait` uses STOP to wait with interrupts enabled, rather than a foreground
  busy loop. Preserve the existing game's interrupt/input owners when adapting.
- `gp_*`, `gfx_*`, `sy_*` and `lab_*` are project routines, not SDK APIs.

Source: [Megadev](https://github.com/drojaazu/megadev), selected image's `lib/`
headers and startup assembly. No SGDK sprite, task, input or resource API is used.

## Main, Sub and Word RAM

The standalone Lab reuses `megadev-system`'s command/ownership implementation
unchanged. C starts a 256-word transform job with guarded buffers. Main polls
ownership and committed replies without blocking movement. A timeout drains the
late job; it does not free or force ownership of its Word RAM. Sub advances one
bounded chunk per observed INT2 tick while preserving BIOS CD servicing.

All collision, actor state, sprite snapshots and resident art used by gameplay
remain in Main Work RAM. The renderer never reads Sub-owned Word RAM. The job's
result is optional diagnostic data, not a frame-by-frame movement dependency.
Adding streaming or PCM services requires assigning distinct mailbox fields and
resource leases through their existing owners, not copying another command loop.

## Renderer and memory budget

The standalone renderer owns VDP writes. Its two Work RAM snapshots contain the
SAT, scroll values and four HUD rows. Publication is one aligned word after all
writes; a pending frame cannot be overwritten. The VBlank consumer checks the
H40 budget (28 project sprites, also conservative scanline admission), defers late
entry and uploads at most 552 bytes. It does not update CRAM during normal play.
Initial palette/pattern installation and map replacement happen with display off.
A byte budget and an early-entry guard are not proof of physical raster timing.

VRAM patterns: font at 0020 (96 tiles), actors at 0080 (92), ground at 0100 (10).
End 010A = 8512 bytes of addressed pattern space, including gaps. Four palettes
are available: UI, shared actors, Garden and Court. One hardware sprite uses one
palette line. Gameplay model is 172 bytes; each frame snapshot is 1644 bytes;
two snapshots cost 3288 bytes. Linker regions reserve Main code/art at
FF0000..FF9FFF, data/BSS at FFA000..FFC3FF, and stack FFD000..FFDFFF, leaving BIOS
state untouched. Inspect actual linked high-water marks when adding content.

Original Main-only PSG music/cues are a Lab convenience, verified from the
[PSG hardware format](https://plutiedev.com/psg), not a Megadev audio API. The Lab
has no Z80 sound driver or other PSG writer. Existing games must route events
through their audio owner; do not introduce another direct-port writer. Pause
mutes the Lab channels. No BIOS, commercial data or downloaded toolchain is
included in the skill.

# Megadev integration and validation

## Verified boundary

The reference container is `ewrogers/megadev:latest`; record `doctor` output for the actual SDK/compiler revision. This milestone checked Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` and GCC 16.2.0. The audited SDK exposes `PAD_*` masks and `IO_DATA1/2`, `IO_CTRL1/2` in `lib/main/io.def.h`; the low byte is SACBRLDU. `read_input_joypad` is a three-button helper, not an eight-player device API. `bios_joy1_*` / `bios_joy2_*` are BIOS state, not a guarantee of extended protocols.

`mp_*`, `in_*`, `gfx_*`, `input_poll` and `coop_*` in this project are application functions. There are no SGDK `JOY_*` or `SPR_*` calls. The copied `in_*` readers match `megadev-input`: AUTO detects pads/Team Player/mouse; explicit EA mode owns both physical ports. Do not mix this callback with BIOS joypad polling. Main owns the ports, snapshots and simulation; Sub cannot dereference Main RAM as though it were shared.

The Lab installs one custom VBlank vector that preserves all registers and triggers BIOS INT2 service (`BIOS_TRIGGER_IFL2`). Input polling happens once in the foreground, with bounded handshake and Z80-bus waits and the caller's interrupt mask restored. The IRQ does not call `BIOS_READ_JOYPAD`. Integrate with the existing game's IRQ/audio/raster contracts instead of overwriting them. A copied reader is not a hardware qualification.

## Renderer and resources

All Lab graphics are resident after startup. Font, 92 actor tiles, 16 ground/UI tiles and four palettes use the ordinary Main-RAM BIOS DMA path while display is off. No Word RAM upload or ownership handoff occurs during play. Do not replace it with a Word RAM pointer without the memory owner and the Word-RAM-specific BIOS DMA adapter.

Two immutable 1,804-byte snapshots contain a bounded SAT, six HUD rows and camera words. Only early VBlank commits them; a late entry retains the pending frame. Maximum published data is 872 bytes (48 entries, 480 HUD bytes, eight scroll bytes); this byte bound is not a cycle guarantee. Palette writes occur only at startup with display disabled. No HBlank handler or active-display CRAM effects are used.

The software sprite list conservatively charges scanline widths and entries. Foreground compilation may continue while the last frame is pending, but cannot modify that published snapshot. Normal play doesn't blank the display or redraw the whole map. The title/play switch is an explicit display-off transition. A single Lab-owned PSG service provides finite cues and a quiet theme; adapt events into an existing audio owner instead of adding a rival driver.

## Evidence

Run `python tools/check_coop.py` in a generated project for O0/O3 host checks with undefined-behavior sanitization. Repository tests add 6,000 randomized roster/camera steps, asset regeneration/palette checks and baseline-input checks. The linked original-68000 qualification executes the same contracts, real input/protocol/MMIO code, Z80 timeout handling, register-preserving IRQ, immutable publication, late-frame deferral and invalid frame rejection. MMIO models do not reproduce pixels, audio or peripheral timing.

The container matrix builds fresh US/JP NTSC release, EU PAL release, US debug, CPU tour and explicit EA variants on ARM64/AMD64. It rejects oversized Main allocations and compares disc/module/boot payloads across architectures. PAL/NTSC simulation uses the gameplay clock's 300Hz units (five or six per VBlank), not different movement tuning. Keep Docker builds at two CPUs.

In ares check title/readability, Start join, movement, A stun/recharge, Start pause/resume, a second configured pad joining mid-run, one keeper returning independently and the shared camera's separation limit. C at the title starts a labeled four-CPU tour that follows an authored circuit, completes the room and repeats after its celebration; C/Start leaves the tour. It is scripted gameplay, not simulated adapter detection. Ares's audited device menu lacks multitaps; real Team Player/EA connections, hot-plug safety, electrical timing, total CPU margin and Z80/audio contention still need real hardware. Never remap a user's controls just to make automation pass.

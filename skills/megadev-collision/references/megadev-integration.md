# Megadev integration

The example targets [Megadev](https://github.com/drojaazu/megadev), verified at `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, through `ewrogers/megadev:latest`. Inspect the selected SDK's headers/source before adapting hardware calls. The collision math is portable C; Megadev supplies startup, BIOS input and the hardware environment.

Main reads `bios_joy1_hold`/`bios_joy1_hit` from `main/bios.h`; the button masks come from `main/io.def.h`. The Lab latches input edges once and advances a 60-step simulation with a 300 Hz accumulator: five units per NTSC frame, six per PAL frame. Catch-up is capped at four steps. Pause stops simulation, not the Main VBlank callback.

Collision remains on Main; Sub only completes startup/Word RAM ownership handoff. There is no invented Sub collision service, per-body job, heap or timer. For a game using a Sub spatial job, follow `megadev-system` and `megadev-memory`: immutable owned inputs, generation IDs, bounded results and safe cancellation/draining before retirement.

The renderer reuses the gameplay Lab's sprite admission and interrupt bridge. Two immutable 1,640-byte frames contain SAT work and four HUD rows. An early-VBlank callback uploads at most 32 SAT entries plus 320 HUD bytes and an optional 48-byte wall-column patch (624 bytes total), or holds the pending frame when late. Foreground never overwrites a pending frame. Invalid counts fail closed. Camera transforms belong after collision; debug rays/contacts are presentation only.

VRAM: font at 0x0000, 13 actor patterns at 0x1000, 23 deduplicated background patterns at 0x2000; Window at 0xA000, SAT at 0xB800, horizontal scroll at 0xBC00, planes at 0xC000/0xE000. Art and collision walls share `res/layouts.json`, so the displayed surfaces agree with queried boxes. View changes install their map with display disabled and present a new frame before enabling it. Raising/lowering the Shots wall instead stages 24 words for one map column and commits them alongside the sprite/HUD frame, without blanking the display. Runtime VBlank does not change CRAM.

The short collision/reset cues write PSG on Main. A project with an audio driver must route these requests through its existing audio owner instead; do not install a second PSG writer.

Use `megadev-gameplay` for the game clock, ordinary tile movement and object lifetimes; `megadev-animation` for hitbox timing; `megadev-graphics` for VDP budgets; `megadev-performance` for measurements. This skill adds precise geometric queries and response contracts without replacing those systems.

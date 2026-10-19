# Megadev integration and validation

The Lab uses real `main/bios.h`, `main/io.def.h`, `main/vdp.h` and `system.h`
interfaces. Its saved-register C adapters call `BIOS_DMA_XFER` only for initial
resident art. One project VBlank path submits immutable SAT/HUD snapshots,
then calls `BIOS_TRIGGER_IFL2` and `BIOS_READ_JOYPAD` and latches hit edges.
`bios_joy1_hold` supplies held direction input. The Sub bootstrap uses the
existing CD service to load Main, then remains available for BIOS INT2 service.

The verified SDK revision is
[Megadev 7a7246c](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib).
Reinspect the selected `ewrogers/megadev:latest` source before changing these
contracts. No SGDK task, input, sprite or pathfinding API is used.

The portable model is C17 with explicit 8/16-bit storage and 32-bit intermediate
products for grid traversal. A small target adapter emits base-68000 `MULU`
for word products; tests include its maximum 32-bit result. This avoids an
implicit `__mulsi3` dependency in the freestanding debug link. No floating point, heap allocation or recursion
runs in gameplay. Compile for the original 68000 with the container safeguards;
inspect generated helpers when profiling, rather than assuming C source cost.

## Meaningful checks

- O0/O3 executable contracts cover facing, occlusion, touching corners, state
  transitions, stun cooldowns, pause, finite unreachable searches, stale results,
  work caps, collision bounds and equal simulated time at PAL/NTSC rates.
- An independent Python BFS checks 80 deterministic random maps. Validate every
  route step, blocked cell, endpoint and length, not just its checksum.
- Execute the compiled model and VDP publisher on original M68000. Confirm Main
  BSS stays below the stack, snapshots remain immutable, late VBlank defers,
  invalid SAT counts fail, and normal publication never writes CRAM.
- Build US/JP/EU release, US debug and automatic qualification discs for native
  ARM64/AMD64 CI and compare complete target payloads. Installation tests keep
  telemetry off and run a fresh standalone example.

In ares, check walls/facing/alert and stun feedback, path detours, body and
projectile hits, all three views, reset, pause, goal and audio. Repeat PAL/NTSC.
Emulator/controller acceptance, CPU instruction execution and host tests are
different evidence. They do not qualify physical VDP timing, real sound quality,
actual drive latency, a larger enemy count or optional Sub offloading.

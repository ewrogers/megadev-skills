---
name: megadev-effects
description: Build bounded particles, explosions, trails, weather and screen shake in Megadev Sega CD games, with effect priorities, resident art and measured rendering budgets.
---

# Megadev effects

Effects add feedback to the game's existing mechanics and art direction. Adapt
these techniques to the user's brief; the Lab's lantern, weather, colors, sound
and controls are illustrative.

- Read [Pools and simulation](references/simulation.md) for finite lifetimes,
  deterministic timing, burst admission, pause and decorative degradation.
- Read [Rendering and performance](references/rendering.md) for resident tiles,
  SAT/scanline budgets, presentation-only shake and one VBlank publisher.
- Read [Validation](references/testing.md) for behavior tests, original-68000
  execution, ARM64/AMD64 comparison, regional checks and measurement limits.

Use `ewrogers/megadev:latest` and inspect the selected image's source. Megadev
exposes VDP and BIOS facilities; this skill's `fx_*` functions are application
code, not a particle engine supplied by the SDK. Do not introduce SGDK's sprite,
DMA queue, resource compiler or task APIs as Megadev calls.

Essential choices:

- Reserve room for gameplay and important impacts before decorative weather.
  Bound particles, emission per update, simulation catch-up, submitted sprites,
  per-scanline work and bytes published. A finite pool alone is not a frame budget.
- Keep transient effects allocation-free when a fixed pool fits. Define the
  full-pool behavior explicitly: reject, replace a less important effect, or
  reduce emission. Never evict gameplay entities to show decoration.
- Use the game's existing clock and input edge handling. Pause and slow motion
  are deliberate policies; do not multiply speeds by the measured render FPS.
- Keep effect art resident where useful. Animate by choosing existing tiles;
  account for remaining VRAM, palette and SAT costs even if no tiles are uploaded.
- Build an immutable frame snapshot outside interrupts. Submit through the
  existing VDP owner, in a measured safe interval. Avoid active-display palette
  writes and unbounded work in a callback.
- Apply shake to camera presentation, not simulation positions or collision.
  Keep HUD panels stable and restore scroll offsets when the envelope ends.

The standalone [Effects Lab](assets/effects-lab/README.md) demonstrates these
choices. `scripts/create_project.py` creates a fresh project without replacing
files. Its separate burst/trail/weather pools and conservative H40 renderer are
one policy, not mandatory capacities or an all-purpose sprite scheduler.

Reuse `megadev-graphics` for renderer integration and palette ownership,
`megadev-animation` for gameplay animation events, `megadev-gameplay` for game
rules, `megadev-audio` for the sound owner, and `megadev-performance` for broader
profiling. Effects must not install competing VDP, drive, clock or audio owners.

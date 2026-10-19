---
name: megadev-combat
description: Implement attack resolution, directional guards, damage, invulnerability and knockback in Megadev Sega CD games. Use for combat rule ordering, attack-instance hit tracking and bounded projectile handling; animation timing, geometry and enemy decisions remain separate concerns.
---

# Megadev combat

Start with the game's rules: who can damage whom, active windows, guard direction,
interruptions, repeated contacts, simultaneous hits and what death removes.
The Lab's court, sword, shields, opponents, controls, music and balance are
illustrations. Do not copy them into a different game unless its brief calls for them.

`co_*` is portable application code, not a Megadev combat API. Inspect the selected
`ewrogers/megadev:latest` headers before using SDK or BIOS services. Keep the game's
existing simulation clock, animation owner, collision geometry and rendering/audio
services; combat consumes their state and emits outcomes.

- [Resolution rules](references/resolution.md): attack identity, hit tracking,
  guards, interruption and deterministic simultaneous outcomes.
- [Bounded integration](references/integration.md): pools, lifetime tokens,
  knockback, Main/Sub boundaries and VBlank publication.
- [Validation](references/testing.md): executable cases, target checks and the
  limits of emulator evidence.

Essential contracts:

- Use simulation poses/events for active hitboxes; never advance combat from
  rendering or sound callbacks. A visible weapon and its active reach should agree.
- Assign an attack instance a lifetime. Track which targets it has contacted;
  define whether a block or invulnerable contact consumes that instance's hit.
  Slot reuse needs a generation or an equivalent retirement rule.
- Specify resolution ordering before optimizing. A stable gather/apply pass can
  permit mutual hits without making actor iteration order decide the winner.
  Decide explicitly whether multiple simultaneous sources stack or coalesce.
- Distinguish hit stun, invulnerability, knockback and guard recovery. Define
  whether hurt/death interrupts an attack and clears its future events.
- Test directional blocks with incoming projectile velocity, not just the
  owner's current position. Reuse swept collision for fast projectiles; this
  Lab's four-pixel step and six-pixel bolt are a bounded discrete example.
- Bound total candidates, pools, catch-up and event storage. On pool exhaustion,
  use the game's explicit policy rather than allocating in a hot loop.

[Combat Lab](assets/combat-lab/README.md) is self-contained and supports a second
configured pad. Install no competing controller scanner or interrupt handler into
an existing game. `megadev-animation` owns poses/events; `megadev-collision` owns
sweeps/rays/sliding; `megadev-ai` owns decisions; `megadev-gameplay` owns clocks,
movement and pools. This skill owns how combat contacts become outcomes.

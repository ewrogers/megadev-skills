# Pools and simulation

## Effects follow gameplay

Emit an impact after a confirmed collision or damage event; emit a trail from an
existing entity's sampled position. Effects are consequences, not a second
collision/damage system. A rejected visual must not cancel a hit, consume a
powerup again, or change an entity's lifetime. Adapt the presentation and tuning
to the game rather than copying the Lab's setting or mechanics.

## Explicit bounded admission

The Lab stores 64 particles of 12 bytes each. Slots 0–23 reserve sparks/smoke,
24–31 reserve trails and 32–63 reserve weather. Admission scans only its partition;
a full partition rejects the new effect and saturates a diagnostic counter.
Weather never occupies the impact reserve. This simple partition can leave
capacity idle; a shared priority pool is also valid if eviction is bounded and
cannot invalidate handles or overwrite gameplay state.

One A edge requests eight radial sparks and four rising smoke puffs. Holding A
does not repeat the request. Trails emit at most once every four simulation ticks.
Weather admits at most two new particles per update and targets 8/16/24/32
particles. Its particles expire or leave the viewport. These are illustrative
limits, not Mega CD hardware capacities.

All simulation loops have fixed upper bounds; no per-frame heap allocation or
compaction is used. A zero lifetime marks a reusable slot. This Lab exposes no
external references to particle slots, so generation handles are unnecessary.
If another subsystem keeps handles, add generation/lifetime checks rather than
assuming a recycled index is still the same effect.

## Time and reproducibility

The frame owner translates video intervals into a 300 Hz integer timebase:
five units per NTSC frame, six per PAL frame. Every five units advances one
60 Hz simulation step. The fractional remainder is retained; PAL occasionally
runs two steps. Velocities/lifetimes/emission cadence therefore share one clock.
The Lab caps a call to four steps and discards excess elapsed time, counting the
clamp. This avoids unbounded catch-up; it intentionally loses simulation time
under a long stall. Choose a different overload policy if the game needs it.

Pause freezes positions, lifetimes, emission, PRNG and the shake envelope. The
renderer/input owner stays serviced. Edge actions are handled once before any
catch-up steps, so a PAL two-step frame does not create two bursts.

The deterministic xorshift state is private to the effect simulation. Rendering
and culling do not advance it. Gameplay randomness should have its own state,
so dropping decoration cannot alter combat or item drops. Q12.4 signed words
cover this Lab's viewport and velocities; wider worlds need camera-relative
coordinates or checked wider math. These types are not a universal world format.

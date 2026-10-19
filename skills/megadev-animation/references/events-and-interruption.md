# Events, interruption and hitboxes

## Advance state, then draw its result

The Lab distinguishes timeline playback from the controller's action rules.
Idle/walk follow input; attack, hurt and rise temporarily lock movement. Hurt can
interrupt an attack; Down+B selects death. A terminal death pose requires a new
A edge after completion to begin rising. These are demonstrative priorities,
not universal combat rules.

Each logical step either handles a new higher-priority action or advances the
current clip. An interrupt does not advance the abandoned clip or deliver its
future hitbox, swish or completion. The new first-frame events are consumed
immediately. A cancelled action has not completed successfully; keep those
concepts separate in a real game's state machine.

`an_step` events are consumed before the controller changes clip. The Lab counts
footsteps, swishes and completions there; its audio bit summary is only a small
per-display cue summary. That summary can coalesce repeated cue kinds and should
not be used as a general gameplay event queue. Run gameplay consumers per step.

For queued effects, record actor/action generation, clip, event and relevant
snapshot data before transitioning. Validate identity again when asynchronous
work returns. Lab generations wrap from 65535 to 1 because no event or handle
escapes the synchronous step. Do not use that wrapping word as a globally unique
lease if a real game can retain stale handles or loaded-frame requests.

## Hitboxes and damage are different contracts

A frame-local half-open hitbox describes the active part of the sword. Its local
X offset and width mirror with the facing direction; body/weapon images use that
same locked direction during an attack. Rendered debug outlines show the actual
box dimensions. Body artwork is 16x24; the smaller body box and dummy box are
separate from the attack's reach.

`an_hitbox` translates only a live active frame, checks representable coordinates
and leaves output untouched on failure. `an_overlap` uses wider intermediate
additions and half-open edges, so touching edges alone do not count. This is an
AABB overlap helper, not a swept collision system or polygonal blade test.

The consumer tests every active simulation step, including poses crossed during
catch-up. Several contacts/poses in one swing can overlap the dummy, but a latch
admits damage only once. The next action clears that latch. A multi-target game
may need a bounded target set or per-target action stamps; do not make the single
dummy's global latch prevent one swing from hitting different intended targets.

Validate spatial reach against art. A cosmetic trail, sprite scale or dropped
sprite must not silently enlarge, suppress or determine the gameplay hitbox.
Fast moving objects need continuous collision or bounded substeps through the
collision owner. Frame hitboxes do not solve tunnelling by themselves.

## Input and callback bounds

The standalone BIOS adapter latches input edges; the controller consumes A/B once
on the first due game step. A pending Down+B snapshot keeps its modifier even if
Down is released before that step. Held A does not restart an attack each frame.
Input during an uninterruptible action is ignored here rather than buffered;
combo buffering is an optional, explicitly bounded gameplay policy.

Keep event processing bounded. Avoid allocation, CD reads, waits, recursive
transitions or arbitrary callbacks in the VBlank consumer. VBlank draws a frozen
snapshot; foreground resolves animation and queues requests through existing
sound/loading services. One frame's event should not become an unbounded chain
of callbacks or a second frame-service owner.

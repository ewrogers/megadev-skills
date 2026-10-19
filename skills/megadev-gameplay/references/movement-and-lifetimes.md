# Movement, collision and lifetimes

## World coordinates and collision

Adventure Lab uses a 512x256 pixel world per area, with a 320x192 playfield below
a four-row HUD. Positions are signed 16-bit quarter pixels; the actor's origin is
its feet. A ten-by-eight-pixel collision box is smaller than its 16x24 image.
The renderer subtracts the camera only after simulation is complete.

`gp_solid` is the single collision-layout owner, also used to draw solid walls.
Outside map bounds is solid. The Lab tests all corners of a box against 16x16
cells. That is valid here because each box is smaller than a cell; larger boxes
must inspect every overlapped cell, including interior cells.

Movement resolves X then Y with at most one-pixel substeps, at most two per axis.
A blocked X can still slide in Y. This order has directional bias; it is a simple
bounded solution for slow walkers, not a contact manifold or swept solver.
The Lab clamps requested displacement to two pixels per axis. Do not apply this
routine to a fast projectile and silently drop its intended travel. Use swept
AABB/segments or another appropriate continuous test instead.

All six gems and all enemy starting boxes are checked against the collision map.
Contact damage has an explicit 90-step invulnerability period. A pulse stuns
nearby enemies and recharges in 180 simulation steps. Stunned enemies do not
inflict contact damage. Sprite blinking does not decide whether damage applies.

## Cameras and area changes

The camera tracks the player within [0,192] horizontally and [0,64] vertically.
The background scroll and actor transforms use the same snapshot, while the HUD
stays fixed. Keep camera easing separate from collision and world position.

Two marked gate openings connect the areas. Only travel toward the edge triggers
a change. The destination places the player inside the opposite gate, so holding
a direction does not immediately bounce back. An area change ends that catch-up
batch. Gems persist per area; enemies reset and transient effects expire.
A respawn restores health and a clear entry position, preserving collected gems.
These policies demonstrate explicit lifetimes, not mandated game rules.

Both maps are code-defined and resident. The transition redraws the small map
with display disabled, then presents a completed snapshot. It is not a CD
streaming example or a promise of seamless map replacement. For larger areas,
use the streaming and scene owners' preload/fade/resource retirement contracts.

## Bounded pools

The four enemy slots are stable gameplay objects, not heap allocations. Twelve
short-lived cosmetic slots are separately bounded: an expired slot is reusable,
and a full pool rejects the newest effect without changing gameplay. Critical
player/enemy sprites are admitted before pickup and cosmetic images. Hardware
sprite rejection must never suppress collision, damage or collection logic.

No external pointer or index survives a cosmetic slot's lifetime in this Lab.
If a real game exposes pooled object handles, add a generation or other lifetime
check before slot reuse; a valid array index alone does not identify the same
object. Defer removals/spawns or establish iteration rules when callbacks can
change the active set. Budget worst-case actors, effects, sprites and uploads
separately, then measure their combined cost on the selected target.

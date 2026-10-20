# Coordinates, surfaces and cameras

The Lab uses integer world units: 256 per map cell, projected as a 32×16 diamond.
One vertical screen pixel is 16 height units. With a foot anchor `(x,y,z)`:

- `screen_x = floor((x-y)/16)`
- `screen_y = floor((x+y-2*z)/32)`

Add the authored map origin, then subtract camera coordinates. The floor operation
matters for negative coordinates: C integer division truncates toward zero. The
example uses a bounded signed-floor power-of-two helper rather than relying on
implementation-defined negative right shifts or an implicit 32-bit division
runtime in a freestanding debug link. These scales are an example, not SDK types.

At a chosen height, the inverse is `x=8*sx+16*sy+z`,
`y=-8*sx+16*sy+z`. Integer projection loses subpixel information; this returns a
representative coordinate, not every original point. Test exact grid points,
negative boundaries, invalid ranges and unchanged output on failure. Picking
across stacked surfaces needs a surface/depth query; a single screen point does
not tell whether the player meant the bridge or the floor below it.

## Movement and elevation

Convert input into world movement according to the game's intended screen-relative
or world-relative controls. Normalize/approximate diagonals to suit its movement
model. The Lab uses a 60-Hz fixed simulation fed by five 300-Hz units per NTSC
frame or six per PAL frame, capped at four catch-up steps. It is a standalone
example; integrate into an existing game clock instead of adding a second clock.

Feet use a small footprint. A surface identifier records whether the actor is on
the upper walkway or ground. Only connected stair approaches change that surface.
The same XY footprint can therefore pass below a deck at ground height or stand
on it at height 512. Upper movement refuses unsupported edges; it does not infer
support from an overlapping sprite. Stair height changes in four authored risers.

World collision excludes decorative deck volumes but blocks posts, towers, trees
and fountain footprints where their height intersects the actor's surface. A
larger game should obtain this information from its existing level/collision data.
This example is not a slope/physics engine. Keep transitions, blockers and visible
stair art in agreement; test entering, reversing at the first riser, crossing,
descending, going underneath and trying to step off the side.

## Camera and anchors

The background is a native 512×256 image in a 64×32 name table. The visible play
area is below a four-row Window HUD. The camera clamps to authored X bounds
64..128 and Y bounds 0..32, so the visible bottom never reads past the map. Sprites
and Plane B use the same camera snapshot; Window text remains fixed.

Each art kind has an explicit foot anchor, independent of canvas size. Hero
frames share a 24×32 canvas and anchor; animation cannot shift their world feet.
Tall 32×64 towers and 16×40 posts are split into ≤32-pixel-high hardware pieces,
with each piece packed in its own column-major tile order. Cropping or splitting
an image after packing it as one tall hardware sprite breaks the tile stride.

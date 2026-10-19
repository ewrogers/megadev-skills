# Water, reflection and split-screen

See [VDP registers](https://plutiedev.com/vdp-registers) and
[VDP commands](https://plutiedev.com/vdp-commands) alongside the selected Megadev
headers. These are ordinary Mega Drive VDP effects, not extra Mega CD colors.

## Resident reflection

The water Lab keeps a 512×256 dry map on Plane B, a vertically reversed map on
Plane A and separate dry/wet palette banks. Reflected entries reverse the map's
tile rows and set each pattern's vertical-flip bit. Patterns are shared; the
reflected name table costs 4 KiB, not a second copy of every pattern.

Plane A begins on a blank table. At the boundary the handler selects the
reflected table and changes VSRAM. For waterline `s`, screen row `y` samples
source row `2*s-y`. The Lab's reversed 256-row map therefore uses offset
`(255-2*s)&255`. It restricts s to 112..176 so visible reflected rows remain
within that authored image. Re-derive bounds for other image/view dimensions.

A 224-row HScroll table gives only the reflected plane a small wave offset;
the dry plane retains its camera offset. The line, scroll table and opaque foam
strip use the same accepted snapshot. Toggling waves is a comparison, not a
change to the reflection geometry.

Banks are allocated deliberately: 0 HUD/foam, 1 dry world, 2 prepared wet world,
3 actors. Only actor colors 4/5 optionally change at HBlank, then restore early
in VBlank. Those slots affect every actor using them below the boundary; they
are not per-sprite private colors. A tint-off mode keeps the reflected map.
Avoid full-bank CRAM copies during active display. Even a small burst needs
real-hardware checks for CRAM artifacts.

## Two views of a shared world

The split Lab uses one resident map and two camera states. Line HScroll selects
each view's camera X, while HBlank switches VSRAM from the top camera to the
bottom one. A coordinate is transformed by its view's origin and camera;
simulation/world coordinates never change just because the display is split.

A scene with more entities must clip each view's projected sprites, duplicate
only entities visible in both, and budget the combined SAT. Sprites do not
inherit background scroll. Use explicit clipping/decomposition, or reserved
opaque boundaries; the VDP cannot scissor an arbitrary sprite to a viewport.
The Lab's two actors remain inside their own view. It demonstrates camera
transforms, not a general clipping engine. B disables the divider and both
scroll splits, using P1’s camera for a single-view comparison. Fully visible
actors are projected into that camera; partial actors are omitted.

## Seam and resource limits

Ten 32×8 opaque sprites cover 320 pixels at the seam. They go first in the SAT
and already consume the H40 per-line sprite-pixel allowance on those rows.
The water actor is decomposed into 8×8 cells; cells touching the strip are
omitted deliberately. Do not rely on hardware sprite overflow to clip it.
A wider mask or additional seam effects must be budgeted, not simply appended.

VRAM byte ranges: font 0400..0FFF, 28-tile atlas 1000..137F, world patterns from
2000 below 8000, reflection map 8000..8FFF, Window A000..AFFF, SAT B800 (640-byte
reserve), HScroll BC00..BF7F, dry map C000..CFFF, blank A table E000..EFFF.
The static generator verifies pattern count; host tests reconstruct the native
image and reflected mapping. If art exceeds these reservations, re-budget VRAM.

The ball reflection is a separately projected sprite, not part of the mirrored
background. Dry 8×8 source cells receive vertically flipped copies at
`2*waterline-source_y-8`, with one shared wave displacement sampled at the reflected ball's center.
Do not displace each 8×8 cell independently: that tears a small reflected actor
into sliding halves even when the background's row-wise ripple looks natural.
Only cells entirely outside the foam and visible below the surface are admitted;
underwater source cells are not mirrored again. The wet actor colors apply to
the reflected cells too. Up to 18 SAT entries include foam, ball and reflection.
For other entities, define reflection culling and priority rather than doubling
all sprites without a budget.

No heap, new drive job or ASIC job is needed for the steady-state effect.
Integrate through the existing renderer rather than installing a second DMA,
clock, sprite, palette or audio owner. Scenery and quiet PSG notes are examples;
the technique does not prescribe water, a city or a game's visual direction.

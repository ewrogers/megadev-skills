# Partial ordering and occlusion

A screen-Y key can work for flat, small actors, but elevation and tall scenery
need more information. The Lab represents each logical drawable with a bounded
world-space box and stable ID. First test overlap of the projected box silhouettes
using the three separating-axis intervals `x-y`, `x-z`, `y-z`. Mere overlap of
screen rectangles creates false constraints across empty corners.

For overlapping silhouettes, a box wholly behind another along X, Y or height
can impose a before/after relationship. Contradictory or interpenetrating proxy
volumes are reported as ambiguous. Authoring smaller drawable pieces, adjusting
proxies or adding a meaningful support relationship may resolve them; a larger
single texture or a different arbitrary sort key does not solve cyclic geometry.

## Explicit links and bounded sorting

A link names stable IDs, not transient array positions. It overrides inference
for that pair; the Lab uses this when the actor overlaps a stair proxy while
standing on its supported steps. Deck/actor ordering above versus below is inferred
from their height intervals. Missing IDs, duplicate IDs, invalid boxes, reverse
explicit links and counts over capacity fail without installing an order.

The sorter holds at most 24 items/24 explicit links. It builds bitset edges,
computes indegrees and emits a topological order. Among available unrelated items,
smallest ID wins, making the result independent of input array permutation. A
cycle produces a bounded diagnostic fallback permutation and a cycle status; it
is not evidence that the scene's occlusion is correct. Callers decide whether to
retain the previous order, show an authoring error or use a documented fallback.

The sample caches a private copy of 15 static objects and their edges. Only the
hero's relationships are recomputed, followed by bounded ordering. Rebuild the
cache if static positions, volumes, IDs or connectivity change. Do not let a
moving platform retain stale cached edges. The cache is 436 bytes on the M68000;
host ABI padding can differ. Never serialize a native C struct as a disc format.

## Emission and cutaway

The software result runs back to front. Earlier Mega Drive SAT entries win sprite
overlap, so emit logical items in reverse order. Keep all hardware pieces of an
item together; a tower's top and bottom cannot independently move around an actor
in the order. Plane priority bits cannot substitute for ordering two sprites.

Treat roofs, bridge decks and cutaway groups as authoring data. The Lab's B button
hides deck/stair drawables while preserving movement, collision and surface state.
C shows a foot marker and status information. This is a debugging/visibility
example, not a rule that every isometric game must reveal its occluded character.

Budget total SAT entries and per-scanline widths, including transparent margins
and all pieces. Verify the authored scene from multiple camera/elevation states;
an item count below 80 alone does not prove the H40 20-entry/320-pixel row limits.

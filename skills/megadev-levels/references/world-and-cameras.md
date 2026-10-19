# Collision, cameras and connected areas

Author collision separately from images. A water picture can be walkable over a bridge; an ornamental foreground canopy should not block movement merely because it hides the character. The Lab stores one collision byte per 16×16 cell and uses a 10×8 foot box independently of the 16×24 character sprite.

The Lab's two one-pixel substeps per 60 Hz walking step bound its supported movement. Each axis is checked separately for sliding. It is not a swept solver and must not be reused for arbitrarily fast movement. Use `megadev-collision` for fast shots/rays or precise time of impact; `megadev-gameplay` owns the game's clock and movement rules.

Imported spawn points are foot centers, with their full foot box checked against map edges and solids. Exit rectangles are world-space interaction regions. Stable area/spawn names resolve at import into bounded indices; do not use editor object order as an external save identity. If saves store indices, version their mapping when authoring changes.

An exit should validate or preload its destination before committing the transition. Activate the new map and spawn atomically, reset stale interaction state and re-evaluate the camera. The Lab uses A while inside a waystone region; automatic travel, confirmation prompts and arrival facing are game choices. Keep arrival outside triggers even when interaction is explicit, to avoid accidental loops if those rules change later.

The gameplay camera clamps to `world_width - 320` and `world_height - 192` pixels. The upper 32 screen pixels belong to the fixed Window HUD. The scrolling renderer keeps a committed camera separate from the desired camera and can follow it by at most eight pixels per presentation. Actors, background patches and scroll registers use the same staged camera, so skipped/late frames cannot disagree about world positions.

Warps, diagnostic-view changes and area changes rebuild the cache with display disabled and enable it only after a new sprite/HUD frame is committed. Ordinary walking never blanks or rebuilds the map. A game can put a fade or loading presentation around those discontinuities through its existing scene owner.

For larger or differently shaped worlds, adapt the importer limits, camera viewport and cache budget together. Do not clamp world coordinates merely because a local collision kernel or a hardware scroll register has a smaller range. The renderer reduces scroll to the hardware's periodic representation after world-space logic is complete.

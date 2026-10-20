---
name: megadev-isometric
description: Implement isometric projection, elevation-aware traversal, depth ordering and occlusion in Megadev Sega CD games. Use for stairs, bridges, underpasses and scenery ordering; the graphics ASIC is optional and does not solve sorting or collision.
---

# Megadev isometric

Start with the game's coordinate convention, footprints, surface connectivity and
camera. Preserve its chosen art, controls, world scale and movement. Lantern
Conservatory demonstrates these contracts; its floating garden, lanterns, palette,
music and bridge layout are illustrative, not a required look or game structure.

`iso_*` is application C, not a Megadev isometric engine. Reinspect
`ewrogers/megadev:latest` before using SDK/BIOS interfaces. Do not import SGDK sprite,
resource, fixed-point or map helpers. Ordinary isometric drawing uses the base VDP;
the Mega CD ASIC may transform selected assets but cannot infer visibility order.

- [Coordinates and surfaces](references/coordinates.md): projection, inverse
  picking, feet, height, stairs, underpasses and camera bounds.
- [Ordering and occlusion](references/ordering.md): projected-volume relations,
  explicit support constraints, static caching and bounded failure handling.
- [Megadev integration and validation](references/testing.md): native assets,
  sprite ordering, Word RAM, DMA, tests and physical-hardware limits.

Keep these decisions explicit:

- Store world position and surface/height independently from screen coordinates.
  Collision uses world footprints and the current surface; sprite pixels are not
  the collision map. Inverse projection needs a chosen height or a surface query.
- Define consistent foot anchors. Split tall/wide art into hardware-sized pieces
  without splitting its logical ordering identity. Transparent margins still use
  scanline bandwidth.
- Screen Y or `x+y` alone cannot order bridges and tall scenery. Build partial
  order constraints for overlapping projected volumes. Explicit surface links
  may resolve deliberate proxy overlap; arbitrary IDs only stabilize unrelated
  objects, not impossible geometry.
- Bound item/link counts and all sorting work. Cache static relationships only
  while their geometry stays unchanged. Report ambiguous overlap, invalid input
  and cycles; do not quietly claim a fallback order is physically correct.
- Keep every piece of an item in the chosen depth order. Megadrive SAT entries
  nearer the beginning win overlap, so a back-to-front software result must be
  emitted in reverse. Plane priority is separate from ordering among sprites.
- Choose an occlusion policy with the designer: complete hiding, a marker, roof
  cutaway or another technique. Hiding graphics must not remove collision or
  change the actor's surface.

Use existing `megadev-gameplay` clocks/movement, `megadev-collision` geometry,
`megadev-levels` map authoring, `megadev-graphics` publication, and
`megadev-memory`/`megadev-system` resource ownership. Isometric adds coordinate,
surface and draw-order policy rather than rival platform owners.

[Lantern Conservatory](assets/isometric-lab/README.md) includes portable contracts,
a cached sorter and a Megadev-native playable demonstration.

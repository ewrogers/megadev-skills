# Palette effects

The Mega CD does not expand the base VDP's four 16-entry palette lines. Transparent pattern index zero and the selected global backdrop need the same care as in a cartridge game. Swapping palettes changes the interpretation of every resident pixel using those entries; coordinate affected actors, maps and UI.

Graphics Lab retains an immutable 64-word base palette. Each effect builds a fresh output instead of repeatedly modifying the prior output and accumulating errors. It commits CRAM only in its VBlank publisher, with no HBlank writes.

| A mode | Example behavior |
| --- | --- |
| Normal | Restore the original palettes. |
| Cycle | Rotate three reserved warm quay colors every ten simulation ticks. Ship and HUD lines remain unchanged. |
| Fade | Subtract a clamped 0–7 amount from each RGB channel on lines 1–3, then return. HUD line 0 stays readable. |
| Flash | Briefly add a clamped amount to lines 1–3 every two simulation seconds, then restore. |

CRAM uses three bits per channel in the word positions documented by the asset pipeline. Shade channels separately and clamp; subtracting a constant from the packed word can borrow into another channel or set reserved bits. The Lab uses integer steps, not RGB alpha blending. Test endpoints, reserved bits, base-palette restoration and unaffected lines.

A real game should reserve cycling entries for intended materials. Do not rotate arbitrary colors just because this Lab has a Cycle button. Independently owned UI/actor/background lines make effects easier to isolate, but palette sharing can be useful when the design calls for it.

## Dithering and shadow/highlight

This Lab's PixelLab sources already fit their exact palettes, with no color reduction or RGB snapping. The distant sky includes source-authored dithering; the foreground and sprite keep crisp clusters. No extra dither pass was needed. If reducing a different background, compare clean reduction with restrained ordered dithering at native size and in the target display. Reject noisy patterns that hurt silhouettes, readability or tile reuse. Do not force dithering on portraits, small sprites or flat UI.

[Shadow/highlight](https://plutiedev.com/shadow-highlight) is also a base Mega Drive feature. It involves priority rules and special sprite colors, not a larger arbitrary palette. A preprocessed image alone does not enable it. It can be useful, but requires a separate art/rendering contract and sprite/VRAM budget. This Lab leaves it disabled and does not perform raster palette tricks.

Palette effects, display ownership and art preprocessing must agree. Keep generation dimensions, palette assignment, conversion reports and runtime interpretation together when adapting techniques; the Lab's dusk colors are illustrative rather than a default style.

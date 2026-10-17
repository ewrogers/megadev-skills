# Source art and palette contracts

## Native pixels first

Agree on what the asset is before generating or converting it: canvas size, visible bounds, side/top/isometric view, frame dimensions, frame order, transparency and palette. Export a static PNG; animated sources should be exported as a regular frame grid for this helper.

A 16×24 sprite is valid: it occupies two by three 8×8 patterns. A 17×19 subject could use a deliberately padded 24×24 canvas. Padding is different from shrinking the drawing; the helper does neither automatically. Inspect alignment and margins when assembling animation cels. Nearest-neighbor enlargement is useful for review, not a reason to deliver an upscaled source as native artwork.

PixelLab can return useful palette-limited native art, but a prompt is not proof. Verify the actual PNG's dimensions, alpha values, palette membership and isolated edge pixels. Look for unwanted downsampling, fuzzy clusters, broken outlines, clipped subjects and inconsistent cels. Regenerate or make deliberate native-grid edits when necessary. Keep the original output, prompt and a short record of changes. Generation styles and the Lab's choices do not prescribe a user's game aesthetic.

## Four palettes, per-asset restrictions

The normal VDP has four palette lines with 16 entries each. A sprite or ordinary background tile selects one line; its pixels contain only four-bit indices. Different assets can use different lines, and a later location can load another palette. Reserve lines according to actual simultaneous use rather than forcing all artwork into one 15-color palette.

Index zero is transparent for ordinary sprite/tile rendering. The visible backdrop comes from the VDP's selected backdrop entry, not automatically from every palette's entry zero. The preparation helper therefore excludes zero for opaque pixels by default.

`--backdrop` is an explicit background-only contract: opaque source pixels may use index zero, and the runtime must select the matching scene backdrop. Asset Lab does that for its bottom background plane. The preview assumes that contract; overlaying the same tiles over unrelated art would behave differently. A sprite cannot use this flag.

Partial alpha has no direct 4-bpp equivalent and is rejected. Flatten it against an intended background or edit the pixels explicitly; do not silently threshold soft edges. Fully transparent pixels become index zero; their hidden RGB values are irrelevant and may normalize in the preview.

## CRAM colors and controlled conversion

The helper uses an idealized eight-step RGB ladder (0, 36, 73, 109, 146, 182, 219, 255) for standard three-bit channels. CRAM words are stored big-endian with red in bits 1–3, green 5–7 and blue 9–11. Display filters, analog output and emulator models can change perceived color; this preview is not a calibrated physical-screen promise.

The palette input is a JSON array of exactly 16 `#RRGGBB` strings. By default, colors must already fit that channel grid. `--snap` explicitly permits nearest channel-step conversion and reports changed source colors. It does not choose the nearest available palette color. A source color still absent from the supplied palette is an error, even with snapping enabled.

This distinction protects highlights and contrast: do not quietly compress an attractive 40-color picture into an unrelated palette and call it done. Rework palette allocation, preserve important light/dark ramps, split appropriate layers or regenerate art for the intended budget. Dithering is a style/tool choice where it helps, not a universal requirement. Raster tricks and shadow/highlight have additional runtime constraints and are not the default asset fix.

Sources: [Plutiedev's sprite guide](https://plutiedev.com/sprites), Megadev's [VDP definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.def.h), and the upstream [prepared resource example](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/new_project/src/ex1_res.s). Verify the source matching the installed toolchain before implementation.

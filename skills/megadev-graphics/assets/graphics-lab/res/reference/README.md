# Original art and preparation record

PixelLab produced four assets for this illustrative dusk harbor. Exact requests are in [requests.json](requests.json); job IDs, dimensions and charged usage are in [provenance.json](provenance.json). The total was **30 generations**. No paid service is invoked by the build or tests.

| Original | Working asset | Preparation |
| --- | --- | --- |
| sky-pixellab.png, 256×192 | sky.png, 512×192 | Original plus horizontally mirrored copy, making both seams meet. Source dithering retained. |
| quay-pixellab.png, 256×192 | quay.png, 512×192 | Same mirror extension; transparent pixels preserved. |
| ship-pixellab.png and ship2-pixellab.png, 32×24 each | ship.png, 64×24 | Cels placed side by side; no scaling. |

All outputs fit the requested native dimensions and exact CRAM color sets. No nearest-color substitution, additional dithering or antialiasing was applied. Visible colors remain unchanged. Mirror extension improves repeat continuity but repeats the composition; it is a practical example, not a substitute for an artist-authored panorama.

The ship's second cel is a subtle propeller/glint change. Prepared previews reconstruct the actual packed patterns/maps, including hardware flips. The explicit palettes and `.asset.json` reports beside each working PNG are the runtime contract. The original font is generated separately by `tools/make_font.py`; it is not PixelLab output or extracted BIOS artwork.

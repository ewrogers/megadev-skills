# PixelLab source record

The exact requests are in `requests.json`; job IDs and edits are in `provenance.json`. Four completed requests used 30 generations in total: two backgrounds at 10 each and two sprite cels at 5 each.

| Asset | Requested / returned | Inspection | Final preparation |
| --- | --- | --- | --- |
| Terrace | 320×160 opaque, specified 16-color palette | Crisp architecture and readable quiet sky; 13 used colors | 438 patterns after flip-aware deduplication; no resizing or color snapping |
| Grotto | 320×160 opaque, specified 16-color palette | Defined rock silhouettes and luminous teal/amber accents | 611 patterns; no resizing or color snapping |
| Balloon cels | 16×24 transparent each, specified 15 opaque colors | Connected outline, transparent one-pixel margin; second cel stays aligned | Two native cels assembled into a 32×24 sheet; 12 contiguous hardware-order patterns |

The original tool PNGs remain here. Active source PNGs live one directory up, alongside reconstructed used previews. Fully transparent pixels encode as index zero; hidden RGB may normalize without altering visible art. The plain 8×8 UI panel is exact procedural data and shares the balloon palette.

The style is demonstration art, not a requirement for games using the skill. These outcomes show why explicit palettes/native dimensions help, but do not remove the need to validate future generations. No generation runs in CI.

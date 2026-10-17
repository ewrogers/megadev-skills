---
name: megadev-assets
description: Prepare and validate images, palettes, tile maps and sprite-frame sheets for Megadev Sega CD/Mega CD projects, then package and load the resulting data. Use for the source-art-to-runtime pipeline rather than SGDK ResComp or a complete renderer.
---

# Megadev assets

Preserve the user's art direction and source pixels. Megadev does not use SGDK's ResComp resource declarations; inspect the project's actual converter, binary layout and loader before adding assets. This skill supports those choices, not a competing game architecture.

## Agree on the asset contract

Identify intended native dimensions, view, transparency, frame grid, palette ownership and where the data will live. Look for the project's existing palette and art rules first. If none exists and color choice matters, ask the artist rather than silently adopting the Lab's palette.

A Mega Drive pattern is 8×8 pixels at 4 bpp. Larger images use multiple patterns. A single hardware sprite supports 8–32 pixels per axis in steps of eight; split larger subjects deliberately. There are four active 16-entry palette lines, not one universal 16-color limit for the entire game. Read [art contracts](references/art-contracts.md) before changing size, color or transparency.

When art generation is requested, use the available PixelLab MCP with explicit dimensions, palette and view. Inspect its current tool schema, follow the user's generation budget and validate the result as you would imported artwork. The Lab's prompts/results are recorded, but its style is illustrative. Do not generate paid art merely to run conversion tests.

## Prepare and inspect

Use [the preparation workflow](references/preparation.md) for the supplied PNG helper, dependencies and regeneration steps. It never resizes, pads or performs nearest-palette reduction. Off-grid RGB snapping requires an explicit flag; missing palette colors and partial alpha are errors. Inspect native pixels and the used preview, not only a scaled screenshot.

For an existing pipeline, retain its formats when practical. The included MDA1 container is an optional teaching format with a bounded C decoder, not a standard Megadev asset type. Read [format and validation](references/format.md) when using or adapting it.

## Package with a lifetime plan

Use [runtime integration](references/runtime.md) when adding build dependencies, disc files, VRAM uploads or palette swaps. Account for packed/prepared data, Word RAM ownership, VRAM tables, shared art and upload time separately.

- Keep shared art resident when that fits the game; load location art without duplicating its shared resources.
- Validate sizes, geometry, CRC and relative pattern references before uploading this format.
- Preserve source buffers until the transfer completes. Ordinary RAM and Word RAM DMA have different BIOS routines; the Word RAM variant handles a documented final-word transfer issue.
- Change palettes and bulk art during a controlled display-off/loading phase unless the project has a measured streaming schedule. Extra CD/RAM capacity does not enlarge VRAM or the per-scanline sprite budget.
- Include source and prepared-file hashes so an edited PNG cannot silently ship stale binary data.

## Prove the path

[Asset Lab](assets/asset-lab/README.md) switches two PixelLab-generated scenes from disc while retaining a shared sprite and UI. Its automatic tour is useful when controller automation is unavailable; it does not test input mappings.

Reprepare the checked-in sources, round-trip pixels/indices, test malformed binaries and run the actual compiled decoder on a 68000 model. Compare native ARM64/AMD64 build payloads, then inspect rendering in an emulator. Separate those results from physical-drive, DMA and display evidence. Project setup belongs to `megadev-project`, lifetime budgets to `megadev-planning`, hardware assumptions to `megadev-hardware`; animation/rendering/streaming systems remain separate work.

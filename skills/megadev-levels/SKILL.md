---
name: megadev-levels
description: Build authored Megadev Sega CD/Mega CD areas from Tiled maps, with separate collision layers, camera bounds, scrolling tile caches and named connected exits. Use for map import/runtime integration and level residency, rather than a general physics engine or genre-specific game rules.
---

# Megadev levels

Start with the game's map scale, camera, collision convention and resource owners. Megadev does not provide SGDK's MAP/resource pipeline. Tiled authoring data needs an explicit importer and a game-owned runtime format; the Lab's `lv_*` functions are application code, not SDK APIs.

## Define an import contract

Use [Tiled importing](references/tiled-importing.md) to choose supported orientations, tile sizes, transforms, layers and object classes. The supplied importer supports a small finite orthogonal profile with a shared 16×16 atlas; unsupported features fail explicitly. Expand that contract when the game needs more instead of silently approximating its artwork or geometry.

Preserve editable source maps and images. Prepare on the host, then verify source/output hashes during container builds. Do not make a normal build regenerate demonstration layouts over a designer's changes. Reject stale prepared data, invalid exits and colors outside the assigned palette before compiling a disc.

## Keep world data separate from presentation

Use [collision, cameras and exits](references/world-and-cameras.md) for collision cells, foot boxes, stable spawn identities and named connections. A visible tile is not necessarily solid. Camera clipping, failed sprite admission and foreground occlusion must never change collision or interaction. Validate destination spawns and keep arrival outside exit triggers.

Reuse the game's simulation clock. Small bounded walking steps can use grid checks; fast objects need `megadev-collision`'s swept queries. Define interaction and arrival behavior from the user's game rather than importing the Lab's controls or progression.

## Bound scrolling and residency

Read [Megadev rendering and residency](references/megadev-integration.md) before changing plane size, scroll registers or resource ownership. Stage incoming rows/columns with the matching camera and sprites; publish them together in VBlank. Late submission must keep the old complete view, not expose partially refreshed tiles. Match the VDP plane dimensions to the cache's wrap arithmetic explicitly.

A small resident world is useful for validating a map pipeline. Larger areas belong behind `megadev-streaming`'s existing CD/preload and Word RAM handoff contracts. Keep shared art resident, validate new data before activating it and drain outstanding readers/jobs before retirement. More disc space does not remove Main RAM, VRAM, palette or upload limits.

## Verify the complete path

[Level Lab](assets/level-lab/README.md) includes editable Tiled maps, native art, independent collision view, foreground canopies, camera tours and connected waystones. Its art, terrain, controls, sounds and traversal rules are illustrative, not a design template.

Follow [validation](references/validation.md): importer rejection cases, an independent cache oracle, compiled original-68000 execution and ARM64/AMD64 regional/debug/tour builds. Review NTSC/PAL in ares and qualify real-console timing separately. `megadev-assets`, `megadev-graphics`, `megadev-gameplay`, `megadev-collision`, `megadev-scenes` and `megadev-streaming` retain their existing responsibilities; add adapters instead of rival services.

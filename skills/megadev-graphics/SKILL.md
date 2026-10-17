---
name: megadev-graphics
description: Render scrolling planes, parallax, sprites, animation frames and palette effects in Megadev Sega CD/Mega CD projects. Use for VDP layout, display priority, sprite budgets and frame submission; asset conversion and disc-streaming policy are separate concerns.
---

# Megadev graphics

Work with the game's existing renderer, clock and art direction. Megadev exposes VDP registers and BIOS services; it does not provide SGDK's sprite engine, ResComp structures or DMA queue policy. Verify the selected toolchain's headers and calling conventions before using an API. The Mega CD still displays through the base Mega Drive VDP.

## Plan what is resident

Identify the configured resolution, table addresses, pattern ranges, palette lines and upload owner before adding graphics. Read [layout and budgets](references/layout-and-budgets.md) when placing art, setting sprite limits or interpreting tile counts. Validate the actual settings rather than assuming the Lab's map is universal.

Keep conversion in the asset pipeline. Graphics consumes prepared patterns, maps, palettes and frame metadata with an agreed layout. A larger sprite or image is not automatically a single hardware sprite, and sprite frame patterns have a different order from a background map. The Lab includes prepared data and its own validator; installing another skill is not required to build it.

## Render through one frame owner

Use [frame submission](references/frame-submission.md) for foreground/interrupt responsibilities, safe publication, BIOS integration and source lifetimes. Prepare immutable frame data outside the interrupt; submit only bounded work during VBlank. Do not load files, decompress, wait or run game simulation in that callback. Disabling interrupts alone does not turn active display into VBlank.

Use [scrolling and sprites](references/scrolling-and-sprites.md) for camera transforms, plane periods, Window HUD, priority and animation timing. Cull before submission, admit important actors first and handle rejected sprites explicitly. Animation is frame selection over the game's clock; it does not require a separate timer per object.

Use [palette effects](references/palette-effects.md) for palette ownership, fades, cycling and flash effects. Preserve the base palette, protect UI colors and commit changes through the same frame owner. Dithering is an asset choice to inspect at native size, not a requirement for every background. Shadow/highlight is a separate optional rendering contract, not extra palette entries that appear automatically.

## Validate the integration

[Graphics Lab](assets/graphics-lab/README.md) demonstrates a scrolling harbor with two parallax planes, animated/flipped airships, foreground occlusion, a fixed HUD and palette effects. Its art, controls and world are illustrative. Adapt the smallest useful technique to the user's game.

Test clipping and boundary cases, links and pattern references, separate scanline entry/pixel limits, palette isolation, frame timing and bounded uploads. The supplied tests execute the actual compiled 68000 model and capture publisher writes; that is not a VDP or timing simulation. Compare native ARM64/AMD64 target payloads, inspect rendering in ares and keep physical-console/BIOS/timing checks explicit.

Project startup belongs to `megadev-project`, preparation to `megadev-assets`, budgets to `megadev-planning` and hardware facts to `megadev-hardware`. This skill owns display integration; it should not introduce a competing scene system, allocator, input driver, animation event system or streaming scheduler.

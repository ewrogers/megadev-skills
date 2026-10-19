---
name: megadev-ui
description: Build bitmap fonts, nine-slice panels, controller menus and fixed HUDs in Megadev Sega CD/Mega CD games. Use for layout bounds, focus and back navigation, readable counters and safe partial redraws; asset conversion, input drivers and scene/resource ownership remain separate concerns.
---

# Megadev UI

Follow the user's visual direction and the game's existing renderer, input and
clock. Menu Lab illustrates techniques; its colors, courier, items, controls and
screen flow are not defaults for a new game. Megadev does not have SGDK's text,
Window or DMA-queue APIs. Check the selected toolchain before using an API.

## Define the layout contract

Read [fonts and layout](references/fonts-and-layout.md) for glyph ranges, fixed
fields, nine-slice corners, portraits and pattern/table budgets. Specify the
resolution, safe margins, text encoding, maximum field widths and font fallback
before rendering. Keep exact native pixels and test the longest actual strings.
Do not silently resize art or assume Unicode fits one tile per byte.

Read [navigation](references/navigation.md) for action edges, bounded directional
repeat, grid movement, back history and focus restoration. Use the input owner's
snapshot and the game's time base. Menus do not need their own device driver or
one timer per cursor. Distinguish a paused game from a paused UI.

## Submit through the renderer

Read [publication](references/publication.md) before adding VDP writes. Prepare
complete immutable work in foreground, publish its commit word last and keep its
storage until consumption is acknowledged. Replace complete fields or rows so
shorter strings cannot leave old glyphs behind. Reserve a bounded allowance for
UI beside gameplay uploads; never give the UI a second interrupt/DMA owner.

Large pages can use a hidden tilemap and switch its base only after all rows
arrive. This needs extra VRAM and staging RAM; it is an optional technique, not a
reason to double-buffer every HUD. Preserve palette ownership and the BIOS VDP
register cache when using BIOS helpers. Interrupt masking alone does not make
active-display VRAM or CRAM writes safe.

## Test in layers

[Menu Lab](assets/menu-lab/README.md) includes two small font faces, large headings,
nine-slice panels, a pack grid, item details, character view, options, pause and
a changing HUD. Create it with `scripts/create_project.py`; its original art,
generator, executable tests and preview tool are self-contained after installation.

Read [testing and APIs](references/testing-and-apis.md) for exact source links,
host/68000 tests and emulator acceptance. Check clipping without adjacent writes,
invalid states, repeated buttons, restored focus, PAL/NTSC repeat timing,
busy/late publications and atomic page switches. Build with the pulled
`ewrogers/megadev:latest`, two Docker CPUs and original-68000 safeguards.

Graphics owns display submission; assets owns conversion; input owns devices;
scenes owns game flow and lifetimes. This skill owns UI presentation and
navigation policy. Detailed dialogue, localization, save management and audio
should use their own contracts rather than being copied into a menu framework.

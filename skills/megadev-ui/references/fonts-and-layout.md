# Fonts, panels and budgets

The base Mega Drive VDP displays 8×8, 4bpp patterns: 32 bytes each. A plane entry
selects an 11-bit pattern index, flips, priority and one of four 16-entry palette
banks. Index zero is transparent; the backdrop uses the VDP backdrop selection.
The Mega CD does not add VDP colors or sprite allowance. Check the actual screen
mode, plane dimensions and table alignment before assigning patterns.

## Text is a resource contract

Declare encoding, glyph-to-pattern mapping, width and fallback. Menu Lab reserves
two 96-tile ranges for byte values 32–127. Its original art covers uppercase Latin,
digits and selected punctuation; unsupported artwork becomes `?`. This is a small
demonstration, not Unicode/localization support. The second face changes highlights.
The 36 large heading glyphs use four row-major tiles each. A hardware sprite may
need another pattern order; do not assume these plane images are sprite frames.

Inspect native-resolution previews. At 320 pixels, one 8-pixel glyph takes one
fortieth of a row. Test actual longest strings and safe margins. Opaque font
backgrounds must match their panel palette; transparent text needs quiet scenery.

Declare every field's rectangle. `ui_text` clips to its width and rejects invalid
rectangles before writing. It does not pad the unused tail: clear the field,
render into a cleared canvas, or use a padded label. `ui_number` writes every
cell, including leading zeros; overflow produces `#`, not adjacent-field writes.
Choose the game's actual number formatting rather than copying this preference.

Nine-slice panels preserve four corners, repeat edges and fill a center. The
example rejects sizes below 2×2. These are tile-sized repeats, not arbitrary pixel
scaling. Portraits/icons have separately reserved ranges and known dimensions.

## Menu Lab's example layout

| VRAM | Owner |
| --- | --- |
| patterns `0x20..0xDF` | Two 96-tile fonts |
| patterns `0xE0..0x16F` | 36 large heading glyphs |
| patterns `0x170..0x17C` | Nine-slice, two cursor poses, two meter cells |
| patterns `0x180..0x19F` | Eight 16×16 plane icons |
| patterns `0x1A0..0x1C3` | One 48×48 portrait |
| patterns `0x1C4..0x1C7` | Four backdrop/floor patterns |
| `0x9000` | Disabled Window base |
| `0xA000..0xAFFF` | Plane B, 64×32 |
| `0xB800`, `0xBC00` | Empty SAT and horizontal scroll table |
| `0xC000..0xCFFF`, `0xE000..0xEFFF` | Two 64×32 Plane A maps |

Visible maps are 40×28 cells; physical rows are 64 cells. VRAM uses a 128-byte
row stride, not the canvas's 80-byte stride. Both Plane A bases obey alignment.
These addresses are example reservations, not universal Megadev defaults.
The native font/art upload is 13,472 bytes; four palette banks take 128 bytes.
Pattern zero and gaps remain reserved.

Main Work RAM reserves code/data below `0xFF9000`, a 16 KiB data/BSS window
through `0xFFD000`, and a 4 KiB stack through `0xFFE000`. Three 2,240-byte canvases
(desired, last acknowledged, hidden-page snapshot) and a 336-byte batch dominate
UI RAM. Read the actual linker map for totals. The BIOS workspace stays reserved.
Small HUDs usually need fewer buffers.

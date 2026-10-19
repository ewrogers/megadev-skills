# Presentation through the UI and renderer owners

The Lab copies the small UI primitives and row publisher from Menu Lab. It removes
unused menu screens, expands the resident portrait range and uses a conversation
generation as the page key. It does not install a second renderer beside a game.
Use the game's existing font/panel and frame-submission interfaces when integrating.

The 320×224 layout has 40 visible columns, with 64-cell physical plane rows.
Main prepares immutable row batches outside its interrupt. Four full rows are
admitted only in early VBlank; late callbacks retain them. A hidden map becomes
visible after seven consumed batches. Register 2 and the BIOS register cache
change together. Normal text/cursor updates replace complete dirty rows, clearing
old glyphs. The UI itself never writes CRAM during active display.

Fonts, headings and panels keep Menu Lab's ranges. Six 48×48 portrait images
(two speakers × idle/talk/blink) occupy patterns `0x1A0..0x277`; four backdrop
patterns follow at `0x278..0x27B`. Patterns stay below the tables at `0x9000` and
above. Plane B, SAT, scroll and double Plane A maps retain the UI layout. Account
for all four shared palette banks; displaying a second speaker does not create
another palette budget.

A portrait's mouth follows reveal timing, not detected speech phonemes. Blink
and continuation indicators use the same clock. This is simple pose animation,
not automatic lip sync. Artists can replace these native images without adopting
the example's colors, costumes or mood.

The second story retains the previous speaker above the current one. Its retained
lines each fit one page. For arbitrary long paired dialogue, store the actual
presented page and portrait state per panel; do not silently show only the first
page of an earlier paragraph. Decide who owns focus and how each panel advances.

Main's example linker reserves `0xFF0000..0xFF9FFF` for code/rodata, a 12 KiB
BSS/data window at `0xFFA000..0xFFCFFF`, and a 4 KiB stack below `0xFFE000`.
The BIOS area stays reserved. Reveal compilation/drawing uses bounded foreground
scratch space; neither is run in the interrupt. Check actual linked totals and
stack high-water before growing scripts, portraits or other game systems.

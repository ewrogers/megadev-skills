# UI work shares the renderer

Do not install Menu Lab's handler beside an existing renderer. Its standalone
adapter preserves registers, triggers BIOS Sub INT2 service, polls pads and
consumes one bounded publication. Integrate with a game's existing VBlank owner.

## Whole pages

`ui_draw` builds a 40×28 canvas outside the interrupt. `ui_prepare` snapshots a
new page and sends at most four rows (160 words, 320 bytes) per publication to
an inactive Plane A map. After seven consumed batches, the final callback changes
VDP register 2 and its BIOS cache. Old content remains visible until then.

This is atomic presentation, not instant rendering: at least seven VBlanks,
about 117 ms at 60 Hz or 140 ms at 50 Hz. On rapid navigation, abandon an unfinished
snapshot only after its outstanding batch is acknowledged, then restart the new
page at row zero. Never flip a partly filled map. A final flip already queued can
briefly show an older page; the game can gate input during transitions if needed.

## Small changes

On the same page/font key, compare desired rows to the acknowledged cache. Scan
cyclically and replace at most four complete changed rows per publication. This
clears old digits, cursors and icon positions. Smaller dirty rectangles/fields
are also valid; preserve complete-field and source-lifetime guarantees.

`UiBatch` stays immutable until consumption. Foreground writes the payload,
issues a compiler memory barrier, then sets a 16-bit commit word. The callback
writes VRAM, records the epoch, barriers, then clears the word. Foreground updates
its cache only after that acknowledgement. This is one 68000 and its interrupt;
it is not a Main/Sub shared-memory or general threading primitive.

## Timing and palette safety

Only admit a row batch in early VBlank: status must report blanking and the
vertical counter must be 224–230 for this 224-line configuration. A late callback
retains the request for retry. That guard depends on the mode and bounded cost;
changes need new measurements. Do not also run the complete BIOS VBlank handler.

CRAM is written only during display-disabled initialization. Cursor animation
selects prepared patterns, so normal UI interaction adds no CRAM updates. Game
fades must join the palette owner's safe budget. Interrupt masking alone does
not prevent CRAM dots. Initial bulk uploads and BIOS calls stay outside the IRQ.

Malformed batches set a sticky fault and remain unavailable for reuse. Production
recovery belongs to the renderer's fault policy. CPU/MMIO tests prove bounds and
ordering, not actual VDP timing or absence of artifacts on physical hardware.

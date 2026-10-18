# Residency, preloads and bounded transfers

Choose what must stay available: player/UI patterns, fonts, current map and any
sound service. Reserve explicit destinations for replaceable content. Extra
Program RAM is useful for resident data, but CPU visibility and ownership still
apply. Disc capacity is not runtime RAM capacity.

Start preloads at a distance or event that allows for **seek + read + validation
+ decode (if any) + upload**. Main/Sub execution overlaps some work; it does not
make the drive faster. Measure on the intended physical drive and account for
retries. If content is late, hold the boundary, retain the current scene, or fade
to a loading state. Never reveal incomplete tiles to keep a nominal deadline.
Avoid frequent speculative seeks; cancel/discard does not recover their cost.

Separate a frame's budgets:

| Work | Streaming Lab example | What to adapt |
| --- | --- | --- |
| Disc destination | One 32 KiB reserved portion of 2M Word RAM | Sector-rounded file limit and owner |
| Validation | Up to 128 bytes of CRC/map checking per Main tick | Actual CPU cost, not just byte count |
| Pattern upload | 64 or 256 bytes per submitted frame | Total VBlank budget shared with HUD/sprites/palettes |
| Map commit | Four 40-cell rows (320 bytes) per black frame | Map layout and transition cost |
| Residency | Shared actor/font; two 640-tile area slots | VRAM/scanline limits remain base Mega Drive limits |

The Lab's publisher copies tile bytes into a Main Work RAM frame before VBlank.
Only after that frame is consumed does `stream_upload_done()` advance its offset.
No Word RAM pointer is queued; no direct Word RAM VDP DMA is demonstrated.
An alternative renderer may pin a source through DMA completion, but a queue
submission is not completion. Account for any decompressor and allocator users
before reusing the input/output buffers.

The two VRAM slots and their maps/palettes become READY only after complete
validation and upload. The displayed area is never the preload destination.
The publisher updates only changed palette lines, before HUD/VRAM writes and
only when the VDP reports VBlank with an early vertical counter (224..231 in
this 224-line Lab). A late callback defers the palette and retries; it does not
write CRAM during active display. HUD updates are limited to 24 changed cells.
This is a measured Lab schedule, not a universal safe transfer budget.

Warp waits for the displayed area palette to be black, installs the selected map while black, then fades
in. Resident UI/actor colors are unaffected. The next-area preload starts during the entry fade; the right edge explicitly
shows EXIT WAITING FOR NEXT AREA if content is still late. Cancelling consumes
that entry trigger; walking back and approaching the exit, or requesting travel,
can start a new load. This is a small **area preload**, not a seamless tile-column streamer or a general cache.

A game may instead use larger chunks, eviction, multiple zones, portals or a
camera streamer. Cache keys should include asset identity/generation; invalidate
READY when replacing its bytes. Prioritize the current view and essential
content before optional cosmetics. The Lab's particular thresholds, controls,
art and fade are illustrative.

Pause suspends the Lab's movement/actor animation but keeps CD/IRQ/validation
and uploads serviced. Pending travel begins after resume. Decide this policy
explicitly in your game rather than pausing the interrupt that drains a read.
No audio service is included; CD-DA/data seek sharing and PCM/graphics contention
remain an audio integration task, not an implied guarantee.

The original demo triggered at x=176 with a boundary at x=294: only 59 movement
ticks at two pixels per tick. Cavern validation needs 166 calls at 128 bytes,
and its 19,552 pattern bytes need 77 calls at 256 bytes. A 59-tick lead cannot
hide even those CPU/upload stages, before adding CD latency. Start earlier,
measure the whole pipeline, and label a late boundary rather than presenting
it as unexplained movement slowdown. Earlier preloading improves the opportunity
to finish; it does not guarantee readiness on arrival, especially in slow-upload
mode or on physical drives.

# Ownership, publication and budgets

## One ASIC job and one uploading snapshot

1. Main validates the request, requires 2M Main ownership and an idle mailbox,
   writes stable parameters, then requests Sub ownership.
2. Main waits for actual Sub ownership before publishing its request token.
   It checks an immediate grant instead of imposing a frame delay. Sub reads
   ownership after observing the committed token, not before it: a grant and new
   request can arrive between two polling instructions.
3. Sub checks the token, mode, owner and idle ASIC, prepares source/map/trace,
   configures the ASIC and writes trace base last.
4. Sub polls completion. While busy it neither modifies inputs/output nor grants
   Word RAM away. It then grants Main ownership, waits for that state and replies.
5. Main requires both the matching reply and a fresh ownership read after that reply before
   copying the completed 8 KiB image into resident Main RAM. It then retires the mailbox.
6. Main VBlank transfers the snapshot into hidden VRAM over two bounded uploads.
   It changes the preview's name table only after the final chunk is complete.
   The next ASIC job may overlap that upload. Its completed Word RAM output waits
   until the old Main snapshot is released before replacing it.

The ASIC stage and upload stage overlap, but the snapshot has one writer and one
bounded reader lease. This does not promise a new transformed image every display
frame: the 8 KiB image alone requires two 4 KiB transfers. The wisp and counters
show independent foreground updates. More buffers or larger transfers need the
game's measured RAM, ownership and frame budgets, not a guessed throughput claim.
The resident copy uses an original-68000 MOVEM block loop; it reads exactly 8 KiB
from Main-owned Word RAM and preserves C callee-saved registers. Changing image
size requires updating and retesting that fixed-size adapter.

A five-second deadline uses 300 Hz units for equal PAL/NTSC duration. A failed
handoff, bad reply or timeout quarantines the request; the current displayed
image remains. There is no unsafe "cancel ASIC and immediately reuse RAM" path.
In 2M mode the lease covers ownership of the whole Word RAM, even though this
profile uses only part of its capacity. Main cannot access the unused remainder
while Sub owns it. This example has no runtime CD reads after bootstrap.
A real game must arbitrate Word RAM with its reader and other users rather than creating a second owner.

## Separate costs

| Pool/work | Lab cost |
| --- | --- |
| Word RAM | Fixed regions listed in ASIC reference; not the whole 256 KiB |
| Sub Program RAM | Source art, trace scratch and transform code; linked bounds checked |
| Main Work RAM | 8 KiB completed snapshot plus HUD/frame state and code |
| VRAM patterns | Two 8 KiB image banks, plus font/wisp |
| Upload per accepted early VBlank | At most 4,096 image + 512 map + 384 HUD + 8 SAT bytes |
| ASIC render | 16,384 output samples; timing must be measured on target |
| First frame | Also initializes 32,896 source bytes and the map |

The HUD queues only changed cells rather than clearing/scanning the entire
40×28 grid on every loop. Queue entries coalesce; shorter dynamic labels pad
their old trailing characters. Frame packets remain bounded and immutable until
consumed. Foreground work rechecks retirement afterward, allowing a confirmed
handshake to progress without an artificial extra VBlank delay.

The two image banks start at tiles 256 and 512. The pending one is invisible
until commit. Main copies Word RAM into a stable Main buffer, so its VBlank DMA
uses Megadev's ordinary `BIOS_DMA_XFER`. It does **not** DMA from Word RAM and
therefore does not use that path's special final-word workaround. A direct Word
RAM DMA design must instead use the audited Word RAM routine and hold the lease
until the transfer really finishes; don't remove the snapshot without replacing
those guarantees.

Only the Main publisher writes display ports during runtime. Palettes are loaded
display-off and remain fixed, so the ASIC path needs no active-display CRAM writes.
The foreground validates its frame packet; the callback defers image DMA outside
VBlank or after line 230. This gate and byte bound are not a universal safe DMA
allowance: BIOS joypad/INT2 work, other transfers, display mode and physical VDP
scheduling still need measurement. Avoid stacking another palette/SAT publisher
or independent BIOS cache update onto the same interrupt.

The transformed output is 4bpp and uses one VDP palette line in this preview.
The ASIC does not increase colors or give each source stamp its own VDP palette.
The HUD has independent palette lines so texture changes do not recolor text.
Transparency, sprite/plane priorities, total VRAM and scanline limits remain VDP
constraints owned by the graphics renderer.

## Seams

- `megadev-assets`: validates native pixels, palettes and resource preparation.
  Scaling adds the ASIC's stamp/trace/output layout.
- `megadev-memory` and `megadev-system`: own existing allocation/lease and CPU
  coordination policy. Adapt the Lab's fixed offsets/request into those owners.
- `megadev-graphics`: owns VDP palettes, tiles, sprites and frame publication.
- `megadev-streaming` / `megadev-audio`: retain CD and sound arbitration; their
  simultaneous operation with ASIC work is not qualified by this quiet Lab.

## Reading the rate

IMAGE/S is a decimal count over the last 50/60 serviced VBlank callbacks. IMAGES
is a cumulative hexadecimal count. The readout is useful for local comparisons,
not a wall-clock profiler or proof that interrupts never miss a physical frame.
The current upload policy alone caps new images at 30/s NTSC or 25/s PAL, before
render/copy/handoff overhead. That ceiling belongs to this 8 KiB/two-upload
configuration; it is not an ASIC-wide maximum. Use a smaller output, a measured
larger upload budget or a different presentation strategy when a game needs a
different cadence. Do not increase the budget blindly or display incomplete data.

## Tuning scaling performance

Measure the complete path before blaming the ASIC. Count newly committed images
separately from serviced VBlanks and game updates. Keep temporary counters for
grant wait, render/reply wait, retirement, pending upload and late-blank deferrals.
Separate the first source-art initialization from steady state. Check that the
profiler/HUD itself is not consuming the time being measured. VBlank counts do
not substitute for an independent elapsed clock when checking missed interrupts.

Start with a transfer ceiling. The Lab's 128×128×4bpp output is 8,192 bytes. With
4,096 image bytes admitted per blank, each complete replacement needs at least
two blanks: at most 30 new images per 60 Hz second or 25 per 50 Hz second, even
if sampling were instantaneous. Also budget map commits, sprites, HUD and other
DMA. For other profiles, calculate the actual padded output stride/extent rather
than assuming width×height/2 always describes the allocated image.

The useful optimizations from this Lab were:

- **Keep static inputs resident.** Prepare stamp art/map once after acquiring the
  runtime layout. Rebuild only trace lines that depend on the requested transform.
  Use bounded fixed-point lookup/reciprocal math, with known signed ranges.
- **Inspect generated copies and arithmetic.** The original word-copy loop used
  expensive indexed address calculations for every word. A fixed-size original-
  68000 MOVEM copy reduces that cost. Keep alignment, exact length, non-overlap,
  C callee-saved registers and source ownership checked. Do not generalize this
  8 KiB routine to arbitrary buffers or assume all C loops need assembly.
- **Overlap independent stages.** Start the next ASIC job while the prior Main
  snapshot uploads. Its finished Word RAM image waits until the old snapshot's
  last DMA releases it. The optimization uses existing buffers; it does not let
  two writers race or reveal a partly uploaded image.
- **Avoid artificial frame waits.** Check a requested grant immediately and poll
  retirement again after useful foreground work. Progress only when the actual
  registers confirm it. A busy loop or a guessed delay is not a substitute for
  ownership. Sample ownership after the committed token/reply: making the path
  faster exposed a stale-snapshot race on both sides of the handoff.
- **Charge UI work to the frame.** Repeatedly clearing/scanning all 1,120 HUD cells
  dominated this small demo even when only a few digits changed. Queue changed
  cells, coalesce repeated changes and cap the published packet. Pad shorter
  labels so the optimization does not leave old glyphs on screen.

Development checkpoints in ares were approximately 5 images/sec initially,
7 after the first copy/overlap changes, 16 with changed-cell HUD work, and 30
with prompt, race-correct handshakes. These are diagnostic observations during
development, not isolated benchmarks assigning an exact gain to each change.
The final decimal rate counts committed images per 60 serviced NTSC VBlanks;
it is neither the ASIC's raw fill rate nor a physical-hardware guarantee.

Recheck identity/rotation/floor pixels and all failure paths after optimizing.
The Lab tests delayed completion, events arriving between ownership and marker
reads, stalled ASIC/grants, protection of the uploading snapshot, exact upload
contents and full-image commit. Confirm the result under the game's real CD,
audio, sprite and DMA workload. If it still misses its target, reduce the output
area/update frequency or choose a measured different upload/buffering policy;
do not remove completion checks or raise the VBlank budget without evidence.

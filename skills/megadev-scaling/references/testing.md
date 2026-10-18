# Scaling validation

## Automated evidence

Host C runs at O0/O3 with undefined-behavior checks where supported. It exercises
all 3 modes × 256 headings × 5 scale settings, identity/quarter-turn vectors,
monotonic floor sampling, signed ranges, output guards, capacity and malformed
requests. Art tests rebuild exact native pixels/stamps and validate palette data.

`tests/scaling_cpu.py` executes actual compiled original-68000 Main/Sub code.
It compares their trace output, checks register write order/units and masking only ASIC INT1, models delayed
ASIC completion and Word RAM ownership, compares identity pixels with the native
art, and checks all three complete image snapshots. Overlapping the next render with
upload cannot replace a snapshot still leased by DMA. A stale reply cannot trigger
a read. Adversarial tests deliver grant/token and handoff/reply between polling
instructions; ownership must be sampled after the commit marker, not cached
from before it. A stuck grant or active ASIC times out without authorizing reuse.
The actual VBlank callback executes with captured port writes and a BIOS DMA
stub: active-display/late calls defer uploads, chunks use the stable Main buffer,
and the tile map flips only after the final chunk. Changed-cell HUD tests check
coalescing, unchanged draws and erasing a shorter label’s stale glyph. No CRAM writes are needed.

The ASIC sampler in these tests is a small functional oracle for the fixed Lab
profile, not an independent hardware-accurate timing model or a GPU implementation
for games. It does not test unimplemented map/stamp profiles or all interrupt
interleavings. Successful CPU execution alone is not an ASIC runtime check.

Fresh US/JP/EU release, US debug and US automatic-tour builds run in the public
latest container on ARM64 and AMD64. Reports require executed adapters on both
CPUs, an ASIC-check marker, identical target payloads and the complete matrix.
CI installs skills with telemetry disabled and checks standalone generation.

## Ares

Open the CUE using `--system 'Mega CD'` with a user-provided BIOS. Check:

- A cycles zoom, orbit and floor; all show coherent art rather than scrambled tiles.
- Left/Right changes heading, Up/Down changes scale, B toggles automatic heading,
  C resets and Start holds/resumes new transform requests.
- Images appear whole, without alternating partial frames. The wisp/counters keep
  updating independently. ERRORS stays zero.
- The floor scrolls in depth and turns; its image update cadence can be below the
  display cadence because the Lab stages uploads.
- The tour switches all three views and reaches AUTO PASS. It must not pass just
  because the Main loop is running without completed ASIC images.

A hold lets already submitted work drain; it does not pretend an active hardware
operation can be frozen or cancelled. Input/control choices are illustrative.

## Still needs real hardware

Qualify ASIC duration, memory arbitration stalls, VBlank DMA/FIFO timing, stack
high-water, BIOS/model differences and PAL/NTSC presentation. Verify combined
CD reading, audio, PCM, other DMA and ASIC work under the intended game's load.
No real-console performance number, general frame-rate guarantee or CD/audio
coexistence result is established by the Lab's model tests or ares observations.

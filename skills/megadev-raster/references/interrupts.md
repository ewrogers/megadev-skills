# Interrupt ownership and timing

## Verify the selected SDK

The audited container uses Megadev commit
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Recheck source after pulling latest:

- [Main memory map](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/memmap.def.h):
  `EXVEC_VBLANK` is the address operand at FF FD08; `EXVEC_HBLANK` is at FF FD0E.
- [Gate Array definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.def.h):
  `GA_REG_HBLANKVECT` at A12006 is the H vector's low word. Do not use bit
  instructions on it. The Boot ROM vector's upper word selects FFxxxx Work RAM.
- [BIOS definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h):
  `BIOS_SET_HBLANK` and `BIOS_SET_HBLANK_WORK_RAM` have different dispatch paths.
  Check the entry/return contract; a direct interrupt handler returns with RTE.

The Lab saves both the full RAM jump operand and the Gate Array word, installs
an even Work RAM handler, and restores both on detach. Installation is idempotent.
Disable interrupts and IE1 through the existing VDP owner before changing either
vector. Do not detach an armed handler or overwrite the game's existing owner.

## Frame protocol

Foreground prepares an unpublished buffer. VBlank accepts a validated buffer
and copies its ten-word plan into dedicated interrupt state. Its scroll table,
SAT and HUD belong to that same snapshot. A word-sized epoch/armed commit makes
publication explicit; the handler never follows pointers into a mutable model.

The VBlank path keeps `BIOS_TRIGGER_IFL2` alive for Sub service, publishes before
`BIOS_READ_JOYPAD`, then latches input edges. HBlank wakes STOP too: foreground
must advance from the VBlank clock, not from the number of wakeups.

At the start of a safe VBlank, disarm the old plan and disable IE1. Restore the
dry palette pair and top scroll/table state before bulk work. Program register
10 before uploads, then arm only if setup still finishes in VBlank. A pending
request can be delivered when IE1 is re-enabled: reject VBlank, blank-edge and
scanlines earlier than the planned boundary without clearing the new arm.

The handler consumes at most one matching epoch. It rejects out-of-window
requests and bounds both halves of its HBlank-edge wait. Each write batch is
preloaded; no status/scanline check is inserted between the fresh edge and the
writes. The water Lab separates table/VSRAM and the two-color CRAM burst onto
successive fresh edges. It checks the epoch again before the latter batch, so
an interrupted old handler cannot corrupt a newly armed frame.

The edge waits and tiny batches briefly mask interrupts at level 7; each poll
loop has at most 64 reads. This is bounded latency, not free latency. Keep the
supported line range away from VBlank, measure the compiled handler, and never
extend the mask over unbounded work. An integration may choose a smaller,
cycle-counted handler after qualifying its exact hardware timing.

## Upload budget and the moving-water regression

A CPU loop copying the scroll/HUD/SAT data ran past NTSC VBlank in the initial
Lab. It programmed/armed the raster at the wrong phase; moving the surface made
the reflection flicker and eventually disappear. A fixed-line screenshot and
instruction-model tests did not reveal this timing failure.

The corrected owner uses `BIOS_DMA_XFER` for Main Work RAM: D0 is encoded VDP
destination, D1 the byte source address, D2 the word count. Its adapter preserves
C callee-saved registers against the documented D0-D3/A6 clobbers. Do not confuse
this with `BIOS_DMA_XFER_WORD_RAM`, which handles the Word RAM DMA erratum.

Per accepted frame the Lab uploads 896 scroll bytes, up to 144 SAT bytes and
512 HUD bytes, plus 8 top/dry data bytes: at most 1,560 data bytes. Register and
command writes also cost time. Two 1,574-byte snapshots consume 3,148 RAM bytes;
this buys immutable DMA sources and a contiguous, stride-correct HUD upload.
Measure safe completion in the combined game, including BIOS service, audio,
CD and ASIC work. Byte counts do not establish a timing allowance.

If setup misses its deadline, do not arm a delayed request. Restore the next
safe frame and count the failure. Do not add an unbounded catch-up loop, reset
counters to hide the fault, or accept a permanent late state as normal.

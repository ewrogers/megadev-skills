# Frame submission and Megadev integration

## One owner of VDP ports

Graphics Lab uses the existing Main BIOS VBlank handler and Sub interrupt service. Its out-of-line `lab_wait` adapter calls `BIOS_VBLANK_WAIT` with **only `BIOS_MASK_DO_USERCALL`**. The default-wait service would also copy the BIOS sprite cache; that would conflict with this example's own SAT publisher. Do not run both sprite owners.

Main prepares a frame containing accepted SAT words, two scroll words, 64 palette words and two HUD rows. It writes only the inactive buffer. Once complete, it publishes the slot with a compiler memory barrier and a brief interrupt-masked 16-bit store. The callback consumes that slot, performs the fixed uploads and clears the publication word. Main waits before preparing the next frame, so the callback cannot observe a half-written frame or a reused source. The VDP retains the previous display if no new frame is published.

This is a small single-producer/single-consumer example, not a general thread-safe queue. Keep its wait/publication invariant or redesign ownership explicitly. Do not scatter independent `volatile` fields across a mutable frame and assume that makes publication safe.

The callback contains no simulation, asset validation, file reads, allocations, nested BIOS waits or display-mode changes. It refuses a count above the Lab's runtime cap. Host and compiled-target checks exercise the bounds and the exact ordered SAT/scroll/CRAM/HUD words, including idle and refused submissions. MMIO capture does not simulate a VDP FIFO, interrupt scheduling or analog display.

## BIOS calls and caches

Megadev headers expose `vdp_ctrl_16`, `vdp_ctrl_32`, `vdp_data_16`, `CRAM_W`, `VRAM_W`, `VSRAM_W` and `to_vdp_addr`. A VRAM byte address is not itself a formatted VDP command. Sequential word writes assume the configured auto-increment of two.

BIOS services have assembly register contracts. The Lab's C ABI adapters preserve D2–D7/A2–A6 and place arguments in the documented registers. Audit each service before changing an adapter; some upstream inline wrappers do not fully document clobbers. Do not assume a wrapper is equivalent to an ordinary C function merely because its declaration is in a header.

Register changes update both the hardware and `bios_vdp_regs`, which stores complete register command words. Palette commits also update `bios_palette`. This example owns CRAM directly in the callback and keeps `bios_vdp_update_flags` clear; it does not simultaneously request BIOS palette copies or BIOS fades. A game using BIOS-owned palette service should route effects through that service instead of copying this second owner beside it.

The custom HUD font is opaque: every pixel is either text index 1 or panel index 2. Window text therefore stays steady over scrolling art. Transparent glyphs need a separate opaque backing strategy; the Window replaces Plane A rather than becoming an independent third full-screen plane.

## Bulk transfers and Word RAM

Initial pattern/map loading happens while display is off. Main validates MDA1 header, geometry, CRC and pattern bounds only after Sub reports successful loading and returns 2M Word RAM ownership. The first sky upload finishes before Sub reuses Word RAM for the quay. Once copied into VRAM, both stay resident; animation selects already resident ship patterns.

Use `BIOS_DMA_XFER` for resident Main RAM and `BIOS_DMA_XFER_WORD_RAM` for a Word RAM source. Both length arguments are in **16-bit words**. The Word RAM service includes the documented final-word data-port workaround. Do not substitute the ordinary source service based on emulator success. Buffer alignment, ownership, source lifetime and a nonzero valid length still matter.

The sample's CRC/load phase is synchronous and may span frames while display is hidden. It is not bounded background streaming. Disc/driver latency, the physical Word RAM DMA quirk and coexistence with CD-DA/PCM need their own tests.

## Freestanding C detail

Megadev links these modules directly rather than automatically supplying every compiler runtime helper. Keep bounded pattern-cell arithmetic in an explicit 16-bit result after validating dimensions; the debug build otherwise requested `__mulsi3` for a promoted expression. The palette's tiny rotation index is narrowed before remainder for the same reason. This is not a compiler defect or a universal rule against multiplication/division. Larger arithmetic can legitimately need a properly linked runtime; test both debug and release output instead of guessing.

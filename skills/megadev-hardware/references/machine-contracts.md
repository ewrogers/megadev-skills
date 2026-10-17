# Machine contracts

The checked Megadev baseline is [`7a7246c`](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef). Read the headers and implementation belonging to the selected toolchain. The Main and Sub include trees describe different CPU views; similarly named registers do not share one address.

## Memory is more than capacity

Main Work RAM is 64 KiB; Sub Program RAM is 512 KiB; Word RAM is 256 KiB. Dedicated PCM wave RAM is 64 KiB. The original VDP still has 64 KiB VRAM, and the Z80 has its own 8 KiB RAM. See Megadev's [program design](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/program_design.md) and the [ares memory allocations](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/mcd.cpp), with [PCM allocation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/pcm.cpp) as corroboration. An emulator implementation is not drive or board timing evidence.

For normal disc boot in 2M mode, Main's Word RAM window begins at 200000; Sub's begins at 080000. The same physical bytes have different addresses. Verify [Main memory definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/memmap.def.h) and [Sub definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/memmap.def.h), not a pointer copied from the other CPU's source.

A Main-side Program RAM window is not permission to read Sub code/data while Sub runs. Follow the bus/arbitration contract before using that window. Hardware Lab instead communicates through the Gate Array and an owned Word RAM buffer. Cartridge-boot/hybrid arrangements need their own mapping audit.

Leave the BIOS reservations intact. [Main BIOS notes](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/main_bios.md) describe workspace starting at FFF700 and the system area at FFFD00. Sub BIOS occupies the lower Program RAM region. Main and Sub each need their own C startup and stack. Module data/BSS initialization and resident callbacks matter even if a memory-size calculation fits.

## Word RAM ownership

2M means all 256 KiB belongs to one CPU at a time. In 1M mode, each CPU has one 128 KiB bank, with swapping and different memory views to consider. Neither mode is ordinary concurrent shared memory.

The Lab uses only 2M mode. It checks the low mode/RET state before either CPU touches the test buffer. MODE is bit 2 and RET is bit 0 in the checked definitions; the Lab masks those two bits, not the register's unrelated protection/bank fields. On return, Main requires 2M mode and RET set before reading data or calling a loaded module.

The actual `grant_2m()` helpers are in [Main Gate Array code](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.h) and [Sub Gate Array code](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.h). They use polling loops. The Lab's command timeout does not bound every BIOS or ownership helper. Do not advertise it as a universal deadlock recovery system.

Publish arguments, then the command, with the required compiler ordering. After acknowledgment, Main clears the command; Sub clears status after observing that clear. Do not reuse the arguments early, infer completion from the first acknowledgment, or treat a timeout as permission to reuse uncertain memory. The Lab stops issuing commands after a runtime failure and asks for a reset.

Memory barriers constrain compiler movement. They do not create ownership, drain hardware work or make an arbitrary C object safe for both CPUs.

## Register and CPU access

The original 68000 uses a 24-bit external address space. Absolute-short instructions sign-extend addresses, so test harnesses must model aliases consistently. Word/long accesses require even addresses. Do not solve an address error by changing the target CPU to a later 68k variant.

Use the documented register access width and preserve unrelated bits. For example, Main and Sub Gate Array registers live in separate windows, and ownership helpers operate on the relevant byte. A blanket 16-bit assignment can affect more than the flag being changed.

The VDP control/status port is not a passive log: status reads can affect pending state and the command latch. Hardware Lab reads its PAL hardware flag once during controlled initialization. That flag reports the emulated/physical VDP standard, not BIOS region compatibility or measured updates per second. Keep 240-line mode, region selection and frame cadence separate.

Megadev's [VDP definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.def.h) name `VDPSTAT_MASK_PAL_HARDWARE`. BIOS calls and callbacks have register/stack conventions too; inspect the wrapper or use an adapter that preserves the C ABI. Avoid changing interrupt masks or installing callbacks without accounting for BIOS services that use them.

## Systems outside this Lab's proof

The ASIC uses Word RAM and needs a source/destination, format, scheduling and ownership plan; it does not draw directly to the VDP. PCM sample memory has banked access and active playback lifetimes. CD-DA and data reads share the drive. Backup RAM must go through the appropriate BIOS file/capacity/error workflow rather than a destructive probe.

Main VDP access, DMA, Z80/FM/PSG access and audio refill remain hardware-sensitive. This skill identifies those contracts; it does not replace dedicated graphics, audio, save or streaming implementation and tests.

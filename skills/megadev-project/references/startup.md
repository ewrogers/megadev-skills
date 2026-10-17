# Startup, memory and ownership

The checked baseline is Megadev commit [`7a7246c`](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef). Verify newer images against their own source.

## Two CPUs, distinct jobs

The Mega Drive Main 68000 still owns VDP rendering and ordinary controller access. The Mega CD Sub 68000 handles the disc-facing program and BIOS services. The add-on provides 512 KiB Program RAM and 256 KiB Word RAM; it does not expand the VDP's 64 KiB VRAM or remove sprite limits. Main Work RAM remains 64 KiB, with BIOS reservations to respect.

The starter boots an initial Main program (IP) and Sub program (SP). SP loads `SPX.SMD` into Program RAM. Main asks Sub to load `IPX.MMD` through 2M Word RAM; IPX relocates itself to Main Work RAM. Later `ROOM.MMD` is loaded into Word RAM and called by Main. MMD and SMD are Megadev module formats, not SGDK resources.

Read upstream [program design](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/program_design.md) and [modules](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/modules.md) before changing this arrangement.

## C needs real initialization

The template invokes `BASIC_INIT` from [init.macros.s](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/init.macros.s) to copy initialized data and clear BSS. Its resident entry also calls `INIT_MMD` to relocate the loaded module. Respect the linker symbols, alignment and module entry convention. The bootstrap's BSS clear converts a byte length to longword iterations; copying a byte count into a longword loop can overwrite neighbors.

Both CPU stacks are explicit. Do not assume the small BIOS Main stack is adequate for C. Ranges below are this example's reservations, with exclusive ends:

| Owner | Code | Data/BSS | Stack |
| --- | --- | --- | --- |
| Main resident | FF0000–FFA000 | FFA000–FFA800 | FFD000–FFE000 (4 KiB) |
| Main room | 200100–240000 in Word RAM | FFB000–FFB800 | Uses Main resident stack |
| Sub application | 010000–01C000 | 01C000–01D000 | 01E000–020000 (8 KiB) |

The room header starts at 200000. These are optional example budgets; increase or reorganize them with a complete memory map, respecting BIOS reservations and other allocations. The linker rejects overflow. A room reload resets its private C data/BSS; resident state persists.

## Calls and interrupts

The room imports resident symbols through its dependency on the resident MMD's ELF. Rebuild dependent modules whenever that ABI changes. Keep VBlank callbacks in resident Main RAM, not a room that might be replaced.

The Lab's C-callable BIOS adapters save the m68k ABI's callee-preserved registers around BIOS services. Consult [BIOS declarations](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h) and assembly wrappers rather than assuming all inline helpers describe every BIOS clobber. BIOS font strings use FF termination and 00 for newline, unlike C strings. The adapter translates the Lab's C strings.

`PROCESS_ACC_LOOP` tail-jumps to the access-loop continuation. Work placed after it in the callback will not necessarily run. The heartbeat increment therefore comes before it. This is covered by an executed callback regression, not inferred from a C model.

## Command and Word RAM lifecycle

This example uses 2M mode: Word RAM has one owner at a time. Main grants it to Sub before a load; Sub transfers it back before signaling completion. Main waits for the handshake, checks `CDROM_RESULT_OK` (0x64 here), initializes the MMD and calls it only after the load succeeds. Do not grant Word RAM away while the room is executing there.

Command arguments precede command publication. The protocol acknowledges a command, waits for Main to clear it, then clears status. The Lab keeps one command in flight and waits at most 180 foreground frames. A timeout leaves the job failed until reset, because ownership/completion may be uncertain. Do not treat the timeout as permission to reuse the buffer. Startup BIOS waits are inherited from the upstream bootstrap and are not all bounded by that runtime timeout.

Volatile registers and a compiler memory barrier establish this small hardware protocol; they are not a general threaded C memory model. Future streaming or 1M-mode work needs its own ownership design.

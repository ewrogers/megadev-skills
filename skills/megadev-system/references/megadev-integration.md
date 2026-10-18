# Verified Megadev integration

Audited against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, selected through `ewrogers/megadev:latest`. Pull the image before checking; record its actual doctor output rather than assuming this revision is permanent.

## Hardware definitions versus project policy

- [Main Gate Array definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.def.h) and [C wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.h): Main writes COMCMD0–7; reads COMSTAT0–7. Main addresses start at `0xA12010` / `0xA12020`.
- [Sub definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.def.h) and [C wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.h): the same mailbox appears at `0xFF8010` / `0xFF8020`, with opposite read/write roles. `ga_reg_comflags_sub` is the Sub-writable byte at `GA_REG_COMFLAGS+1`; the byte at +0 belongs to Main.
- Main `WORD_RAM` is `0x200000` in this disc target; Sub `WORD_RAM_2M` is `0x080000`. These are two views of the same owned memory, not two copies. See each CPU's `memmap.def.h`. Main Work RAM is not a shared C heap.
- `ga_reg_memmode` exposes MODE/DMNA/RET. This Lab admits Main access only when low three bits are 001 and Sub access only at 010. It refuses 1M mode. This deliberately narrow convention does not teach all legal 1M bank configurations.
- Both `grant_2m()` wrappers loop until confirmation. They remain in the bootstrap copied from Project Lab. Runtime job handoffs instead make one byte-level request and observe confirmation on later updates. Those bounded adapters are project code, not an asynchronous SDK grant API.

C `volatile` ensures MMIO accesses are emitted. The compiler barrier before a commit also keeps ordinary buffer preparation on the correct side of the handoff. It is not a replacement for a wire protocol or a hardware ownership check. The 68000 has no data cache to flush here; do not invent SGDK cache/DMA APIs.

## BIOS and interrupt coexistence

The [Main BIOS notes](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/main_bios.md) explain that the default VBlank handler calls `BIOS_COMM_SYNC`, palette service and input polling. COMM_SYNC includes a cache/flag protocol as well as the Sub INT2 pulse. Do not mix a direct mailbox owner with cached writes to those same fields.

The standalone Lab installs one handler after bootstrap. It saves registers, calls the verified `BIOS_TRIGGER_IFL2` and `BIOS_READ_JOYPAD` entry points, then its own bounded VBlank publisher. It queues input edges and counts every VBlank even when the UI is slower. Normal display has no independent BIOS palette/sprite-copy path. An existing game should integrate into its own handler instead of installing this one over it.

The Sub bootstrap's `sp_int2` increments its heartbeat **before** `PROCESS_ACC_LOOP`, which tail-jumps to the access-loop continuation. Code after that macro is not a reliable place for a heartbeat or worker. Background chunks run in Sub foreground with interrupts enabled, never inside the CD access callback.

## Startup boundary

Project Lab's initial load uses its original CMD_LOAD_FILE handshake, `load_file(CDROM_LOAD_CDC,...)`, `access_op_result`, `grant_2m()` and module initialization. System Lab switches to the new token protocol only after IPX has loaded and that bootstrap handshake is fully retired. Token 1 stays reserved for bootstrap. Do not change one CPU's format while the other still speaks the old format.

Boot loading is still synchronous and is not the bounded runtime job demonstrated here. The Lab does not establish async disc reads, preemption of BIOS calls or sound-driver compatibility.

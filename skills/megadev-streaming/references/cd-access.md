# Actual CD access and safe staging

Audited against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`:
[CD documentation](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/cdrom.md),
[header](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/cdrom.h),
[assembly](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/cdrom.s),
[macros](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/cdrom.macro.s).
Inspect the selected image again if its revision differs.

`INIT_ACC_LOOP` primes a resident Sub service. `PROCESS_ACC_LOOP` resumes it
from Sub INT2; its macro tail-jumps to the saved continuation. Put other callback
work before it and honor clobbers. Main must keep triggering Sub's INT2 even
while paused, fading or loading. Do not replace the game's existing IRQ owner.

At startup, `CDROM_LOAD_FILE_LIST` caches root directory metadata. The default
cache holds 128 entries; audit the parser/cache before raising file count or
using directories. The Lab uses a small root-only data disc. Filenames are
ISO9660 Level 1 uppercase 8.3 strings including `;1`, such as `FOREST.MDA;1`.

`CDROM_LOAD_CDC` uses BIOS CDC transfers to a Sub-visible absolute destination.
For an asynchronous request, set `filename` and `filebuff` while the service
cannot observe a partially updated request, then commit `access_op` last.
Wait across foreground ticks for `access_op == CDROM_IDLE`; check
`access_op_result == CDROM_RESULT_OK` and the resulting `filesize`.
`CDROM_RESULT_NOT_FOUND` and `CDROM_RESULT_LOAD_FAIL` are distinct failures.
These are actual symbols. The `load_file()` C convenience wrapper blocks its
caller until idle and returns a byte count or zero, not the result code. Its
pinned implementation assigns `access_op` before the parameters; the Lab uses
an explicit parameter-first request instead.

The reader writes entire 2048-byte sectors, including the final sector padding.
A 2049-byte file needs 4096 writable bytes. Reject zero, arithmetic overflow and
`ceil(file_bytes/2048)*2048 > capacity` before touching the reader. A checksum
after loading cannot undo an overflow. For generated content, check the directory
size against a build-generated manifest as well as the runtime buffer capacity.

The assembly `find_file` returns A0 pointing at the cached record, with carry
set on failure. The size field is at byte offset 18; sector offset is at 14.
The Lab's `file_info.s` is an original C ABI shim that preserves callee-saved
registers and returns size or zero. It avoids the pinned `find_file_c` inline
wrapper (which contains RTS in C inline assembly) and C struct-padding assumptions.
Do not call it before the directory is initialized or while another request
owns the directory/reader state.

This slice uses **2M ownership**: Sub loads at its `WORD_RAM_2M` window, returns
the region and reports completion only after Main ownership is visible. Main
reads its `WORD_RAM` window after confirming the matching command and mode.
Neither CPU runs code/stacks from that staging region. A 1M pipeline needs a
separate audit of destination mapping and CDC behavior; a successful 2M example
is not evidence for 1M CDC DMA. The memory skill's bank-swap example alone does
not qualify concurrent disc transfers.

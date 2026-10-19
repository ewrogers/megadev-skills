# C and assembly at the boundary

## Selected source and compiler

Verified against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` and the selected
container's GCC 16.2.0/original-68000 target. Recheck `doctor` and the actual build
commands after toolchain changes; `latest` is a moving reference, not a version.

- [megadev.make](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/megadev.make): module compilation/linking; bare module links do not automatically provide every runtime arithmetic/library helper.
- [system.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/system.h): interrupt helpers. Inspect their actual SR behavior; do not assume `enable_interrupts()` restores a previous mask.
- [Main Gate Array declarations](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.h) and [Sub declarations](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.h): volatile register width/direction and CPU-specific addresses.
- [Sub BIOS definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bios.def.h): BIOS constants, not C function signatures invented from their names.

Keep `-fno-strict-aliasing` and the container's instruction/optimizer safeguards.
They are build policy, not permission to use invalid C. Inspect undefined symbols
and target disassembly when adding wide arithmetic, library calls or assembly.
A host test cannot show whether a missing libgcc helper will link on target.

## Width, promotion and defined arithmetic

Use byte/word/long types deliberately and assert their required sizes. The chosen
m68k C ABI uses 32-bit `int` and pointers, unlike a host's pointer width; don't
serialize host `sizeof`, pointers, enums or raw structures as a portable file format.
A word type still undergoes integer promotion before many operations. A product
of two unsigned 16-bit operands can overflow a promoted signed 32-bit `int`.
Cast before arithmetic when a wider unsigned result is intended, and verify that
the resulting operation links and is worth its target cost.

Signed overflow is undefined. Unsigned wrap is defined, but useful only when the
protocol's wrap assumptions hold. Use subtraction-based range checks (`length <=
capacity - offset` after `offset <= capacity`) rather than a sum that can wrap.
Do not left-shift a negative value or depend on signed right-shift behavior for a
portable codec. Check shift counts and divisors before use.

`contracts.c` demonstrates bounded slices, explicit big-endian decoding, a
validated checkpoint domain and an elapsed-time conversion with a capped result.
The host checks exercise O0/O3 with an undefined-behavior sanitizer where supported;
compiled tests read an odd-address byte stream on the original 68000.

## Alignment and aliasing

Original 68000 word/long accesses need appropriate alignment. A packed file is
not an aligned C structure: decode its bytes, validate lengths before reading,
and keep output unchanged on failure. `bp_read16` is an application example,
not a Megadev API. Align assembly tables/blobs where word/long consumers require
it, and distinguish a CPU address from an offset into an asset.

Avoid incompatible-pointer type punning. Character-byte access and explicit
field assembly are clear at file/wire boundaries. If using `memcpy` for a local
representation, ensure the target build supplies or inlines it; do not invent a
library guarantee. Keep wire/file endianness explicit even when both CPUs share
an endianness today.

## ABI and interrupt obligations

C declarations must match the real calling convention and return width. The
bundled Sub BRAM wrapper preserves callee-saved registers, reads C stack arguments
at the offsets established by its register save, translates BIOS carry/status
into C values and returns with the original stack. Its compiled regression model
executes that wrapper and checks stack restoration, metadata and exact filenames.
It is reused from the save owner, not an alternative public BIOS interface.

For inline assembly, declare input/output registers, condition-code clobbers and
memory effects that actually occur. Do not hide a BIOS call in a small asm string
that leaves its ABI obligations invisible. Interrupt entry/return is a different
contract from a normal C call; use the project's existing bridge rather than
installing a competing handler.

A volatile 32-bit value is not automatically an atomic observation across Main
foreground and interrupt code on a 16-bit bus. Use a suitable snapshot/critical
section or commit protocol. A compiler memory barrier orders compiler-visible
accesses; it does not grant Word RAM, acknowledge a device or wait for completion.
Do not introduce C atomics on MMIO without verifying the emitted instructions and
hardware semantics.

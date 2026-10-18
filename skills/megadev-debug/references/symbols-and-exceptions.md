# Symbols, module identity and exceptions

Build in a fresh generated directory with:

```sh
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build DEBUG=1 'OPT_FLAGS=-Og -g -fconserve-stack -fomit-frame-pointer'
```

Keep the release build that reproduced the problem too. Optimization can inline,
merge or remove code; a source line is an approximate attribution, not a complete
execution history. `-fomit-frame-pointer` means a guessed A6 backtrace is especially
unsafe. Never walk unvalidated stack addresses to make a crash report look complete.

The Lab's `tools/lookup_address.py` requires an ELF SHA-256 captured with the
failure, an explicit CPU, a known Lab module and an aligned runtime PC inside
that ELF's linked text. It rejects wrong identities and addresses in padding,
BSS or another module. It reports missing source information honestly. It is a
Lab-specific address map: adapt the role/window table for another game.

For example, replace the placeholders with captured values, not values from a
new rebuild:

```sh
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest python3 tools/lookup_address.py build/ipx.mmd.elf --cpu main --module main --pc 0xCAPTURED_PC --sha256 CAPTURED_ELF_SHA256
```

Inspect symbols with `m68k-elf-nm -n`, disassemble the matching ELF with
`m68k-elf-objdump -d`, and retain the region, flags, SDK/compiler revisions and
loaded-module identity beside the hash. This tool checks supplied identity; it
cannot prove that a running machine actually booted those bytes.

Main resident code is linked at `0xFF0000`, Sub at `0x10000`, and ROOM.MMD at
`0x200100` in this fixture. The MMD's 256-byte header is not executable game code.
ROOM.MMD is a startup/symbol qualification fixture, not a runtime diagnostic UI;
its Word RAM code would be invalid after another job overwrites that region.
A breakpoint or saved PC for an unloaded module must not be treated as resident.

## Fatal exception handling is a separate integration

The audited `lib/main/cd_exception.s` is Main-only and expects `str_util.s`.
Its installer changes BIOS user-vector targets. Its fatal display masks interrupts,
changes VDP state and clears display memory; it is not composable with the normal
frame publisher and is not a recoverable gameplay notification.

The source documents a fixed Main bus-error reset path and a shared address-error /
illegal-instruction user vector with incompatible frame interpretation. It also
contains suspicious `exEXVEC_TRACE` / `EXVECEXVEC_TRACE` spellings. Audit and qualify
that assembly before opting into it; this skill does not silently install or
repair upstream exception code. Do not assume it handles Sub exceptions.

A base-68000 ordinary exception frame and a bus/address-error frame differ. Save
CPU role, vector, raw bounded frame bytes, SR, PC, stack pointer and build/module
identity before rendering anything, but only decode fields whose frame layout is
known. A bad stack may prevent even this. Never reuse a 68010/68020 frame decoder
or resume past an arbitrary fault. Debug Lab injects recoverable protocol/data
failures; it does **not** claim verified fatal-vector capture or real-hardware crash recovery.

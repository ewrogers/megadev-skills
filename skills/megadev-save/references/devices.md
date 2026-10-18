# Backup RAM and devices

## Internal storage

Mega CD has 8 KiB of internal battery-backed RAM, shared between games. BIOS
filesystem metadata consumes part of it. Treat usable record capacity separately
from physical size. Ordinary Mega Drive cartridge SRAM assumptions and SGDK APIs
do not apply to this BIOS-managed file interface.

Megadev's audited Sub definitions provide `_BURAM` at `0x005F16`:

| Code | Service | Lab use |
| --- | --- | --- |
| 0 | BRMINIT | Resident 0x640-byte A0 work buffer and 12-byte A1 display buffer; carry clear means Sega-formatted storage |
| 2 | BRMSERCH | A0 points to name; carry reports presence; D0.w reports blocks and D1.b mode |
| 3 | BRMREAD | A0 name, A1 destination; carry reports success; check size/mode again |
| 4 | BRMWRITE | A0 14-byte file-info structure, A1 data, D1.l zero; carry reports success |
| 6 | BRMFORMAT | After BRMINIT and explicit device confirmation only; carry clear means success |

BRMINIT error D1 values distinguish no RAM (0), unformatted (1), other format (2).
Its successful carry result is authoritative; the Lab converts it to status 3.
BRMSTAT exists, but do not conflate its output units with BRMINIT capacity or
record block sizes without verifying the selected BIOS documentation.

Names have 11 ASCII bytes. The Lab stores a zero mode byte immediately after
the name, providing the zero terminator needed by search/read as well as the
write structure's normal-mode field. The final big-endian word is block count.
Normal-mode record blocks are 64 bytes; protected-mode blocks differ. The Lab
requires exactly one normal block and refuses other metadata before reading.

Assembly adapters preserve C callee-saved registers, capture carry before changing
condition flags and restore A0 after BRMSERCH changes it. BRMWRITE explicitly
sets D1.l to zero as documented in Megadev's Tech Bulletin #1 note. The Lab uses
full readback instead of relying on the ambiguous A0/A1 comment in BRMVERIFY's
header. Both CPUs retain their existing startup and interrupt ownership.

Save operations run in Sub foreground, not its INT2 callback. Main stays
responsive and accepts the matching result through one bounded mailbox. Keep the
BIOS work buffers resident. Add game-specific scheduling around other BIOS work;
the Lab does not qualify simultaneous CD music, DMA, ASIC and backup-RAM calls.

## Optional backup cartridge

Sega's consumer manual describes a 1-Mbit cartridge (128 KiB), sixteen times the
internal physical capacity. It fits the Genesis cartridge slot and provides a
separate storage device. Games usually saved internally; the system menu can copy
files between internal and cartridge RAM, and some games support direct cart saves.
Do not promise that inserting a cartridge automatically expands every game's
internal file space. Formatting destroys existing records and belongs to an
explicit user-controlled workflow. The Lab provides one separately from saving.

The Main-side `BRAM_CART` entry definition is `0xFFFDAE`; the audited Megadev
header supplies no complete typed operation wrappers. The Lab adds separate
project-owned Main adapters for init/search/read/write/format after auditing the local
US BIOS. They preserve the C ABI, carry, exact file metadata and D1.l=0.
The runtime calls the boot-installed vector, never a guessed ROM offset.
Main has its own resident 0x640-byte workspace and 12-byte display buffer;
the internal Sub workspace stays separate. Do not put either in a scene arena.

The device selector is explicit. Main checks the lower-byte ID at `0x400001`
and accepts standard IDs 1–6 before BIOS initialization. Physical capacity is
`0x2000 << id` bytes; ID 4 is 128 KiB. A valid ID alone is insufficient: the BIOS
must report formatted media for ordinary file access. Absent/invalid IDs,
unformatted media and failures do not fall back to internal writes. High-bit/custom Work RAM handlers are refused
before their code can execute.
The adapter also requires an absolute-jump vector to an even BIOS-ROM address
below 0x40000. A vector redirected into cartridge/RAM code is refused even if the
current ID looks standard. A different legitimate vector shape needs a separate
audit rather than weakening that check blindly.

The audited Main BIOS uses codes 0/2/3/4/6 with named-file metadata and normal
64-byte blocks. It manages the write-enable latch itself. This ABI was executed
against the supplied firmware on fresh 16/128/512 KiB virtual carts, not inferred
solely from the Sub API. BIOS variants and physical carts still need qualification.
Model address decoding/mirroring from the selected hardware/emulator; a quoted
SRAM window is not a reason to replace BIOS files with hardcoded raw offsets.

Charles MacDonald's hardware teardown reports the official cart's ID at the
400000 region, 128 KiB on the lower byte lane at 680000–6BFFFF, an unpopulated
second RAM area, and a write-protection latch at the 700000 region. Chilly Willy
describes ID-dependent capacities and compatibility differences. These are
hardware observations, not proof that arbitrary ID sizes or custom Work RAM
cartridge drivers work on every BIOS. Do not execute cartridge-supplied handler
code merely because a signature was detected.

The Lab implements **internal and standard-cartridge files** with independent
records. Copying between devices and custom Work RAM handlers are outside its
scope. Ares v148's standard cartridge board does not implement the CD backup-cart
ID/RAM/latch mapping. Use a cart-capable emulator or hardware for an interactive
cart check; successful internal ares saves do not qualify the cart path.

## Explicit formatting

Use the BIOS formatter, not a hand-built filesystem layout. `BRMFORMAT` is code
6 after `BRMINIT`, on Sub for internal RAM or Main's checked `BRAM_CART` vector
for a standard cart. It erases the entire selected device, including other games'
files. This is different from deleting the Lab's two records.

Keep formatting out of startup, normal saves and recovery. Name the device and
warn that all saves there will be erased; default to Cancel. The Lab uses Help's
C to open confirmation, Left/Right to choose, then a separate A press to accept.
B/Start cancel. Navigation combined with A never confirms in the same input.
Device changes and other requests are blocked while a request is pending.

The explicit format operation accepts present unformatted, other-format or
already formatted media. Missing storage and unsupported/custom cart handlers
are refused. Initialize immediately before formatting, capture BIOS carry, then
initialize again to check the result. Report BIOS errors or a failed re-probe;
never show success merely because the command returned. Clear cached record
sequence after the operation and leave working game values unchanged. The
project's request code 5 and `SV_FORMATTED` are helpers, not BIOS function codes.

## Sources

Audited SDK revision: `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`.

- [Megadev Sub BRAM definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bram.def.h)
- [Megadev C wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bram.h)
- [Megadev cartridge vector](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bramcart.def.h)
- [Official cartridge US manual](https://segaretro.org/images/9/9a/Backupram_mcd_us_manual.pdf), introduction and printed pages 2–6, visually inspected from the scan.
- [First-hand cartridge teardown/discussion](https://gendev.spritesmind.net/forum/viewtopic.php?t=826)
- [WindDrake's cartridge hardware project](https://github.com/WindDrake/SegaCD_Ramcart), useful hardware/board context, not a Megadev software driver.
- [Ares v148 Mega CD implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/mcd.cpp), 8 KiB allocation and `backup.ram` persistence.
- [Ares v148 standard cartridge board](https://github.com/ares-emulator/ares/blob/v148/ares/md/cartridge/board/standard.cpp), external-cart capability boundary.
- [Genesis Plus GX CD cartridge implementation](https://github.com/ekeeke/Genesis-Plus-GX/blob/master/core/cd_hw/cd_cart.c), reference for ID, byte lanes, mirroring and write-enable behavior. Its implementation is not copied into this MIT project.

The optional local firmware fixture uses SHA-256
`b1c2036b79467514eaaa69c76e5ba83801621e821059caa53cb921a0b21e3af4`.
No BIOS bytes or disassembly are distributed. The fixture refuses unknown hashes;
production adapters use the installed vector, not the fixture's ROM entry offset.

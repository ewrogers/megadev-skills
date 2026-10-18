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
explicit user-controlled workflow. This Lab never formats either device.

The Main-side `BRAM_CART` entry definition is `0xFFFDAE`; the audited Megadev
header supplies no complete typed operation wrappers. The internal Sub adapters
must not be redirected to that address and assumed compatible. Direct cartridge
support needs an audited Main BIOS ABI, detection, size/bounds and write-protect
policy, plus tests against the actual device/emulator configuration.

Charles MacDonald's hardware teardown reports the official cart's ID at the
400000 region, 128 KiB on the lower byte lane at 680000–6BFFFF, an unpopulated
second RAM area, and a write-protection latch at the 700000 region. Chilly Willy
describes ID-dependent capacities and compatibility differences. These are
hardware observations, not proof that arbitrary ID sizes or custom Work RAM
cartridge drivers work on every BIOS. Do not execute cartridge-supplied handler
code merely because a signature was detected.

The Lab implements **internal files only**. Cartridge archive/copy behavior is
documented; direct cart saving and custom Work RAM expansions are not claimed as
implemented or qualified. Choose and test a backend explicitly before adding them.

## Sources

Audited SDK revision: `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`.

- [Megadev Sub BRAM definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bram.def.h)
- [Megadev C wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bram.h)
- [Megadev cartridge vector](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bramcart.def.h)
- [Official cartridge US manual](https://segaretro.org/images/9/9a/Backupram_mcd_us_manual.pdf), introduction and printed pages 2–6, visually inspected from the scan.
- [First-hand cartridge teardown/discussion](https://gendev.spritesmind.net/forum/viewtopic.php?t=826)
- [WindDrake's cartridge hardware project](https://github.com/WindDrake/SegaCD_Ramcart), useful hardware/board context, not a Megadev software driver.
- [Ares v148 Mega CD implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/mcd.cpp), 8 KiB allocation and `backup.ram` persistence.

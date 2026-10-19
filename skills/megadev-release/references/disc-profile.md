# Validated disc profile

## Source of the contract

Checked against Megadev commit `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`:

- [cd_boot.s](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/cd_boot.s): disc signature, fixed IP extension fields, IP bytes at `0x200`, SP at `0x1000`, boot area padded to `0x8000`.
- [security.c](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/security.c): selected regional security payload. The helper compares its known length/CRC, not a user-supplied BIOS.
- [megadev.make](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/megadev.make): boot binaries, `mkisofs -iso-level 1 -G boot.bin -pad` and disc input directory.
- [IP linker](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/cfg/ip.ld) and [SP linker](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/cfg/sp.ld): loaded RAM origins and link-time capacities. File length alone does not account for BSS, stack or module lifetimes.

The header's `0x800` IP field is not the location of the start of the included
IP binary: Megadev includes it at `0x200`. Do not use that field to hash the wrong
slice. Compiled `ip.bin`/`sp.bin` are compared against their actual disc positions.

These are verified Megadev conventions, not a claim that every commercial disc
uses this layout. Re-audit them if the SDK changes. Security CRC identifies this
known block; it is not a cryptographic signature or a universal BIOS acceptance test.

## CUE and audio

The helper accepts a narrow multi-file layout:

- Track 01 is one `FILE ... BINARY`, `TRACK 01 MODE1/2048`, with `INDEX 01 00:00:00`.
- Subsequent consecutive tracks are separate `FILE ... WAVE`, `TRACK nn AUDIO`.
- Audio is uncompressed stereo signed 16-bit 44,100 Hz WAV, a whole number of
  588-sample CD sectors (75 sectors/second), with at least four seconds of program
  after INDEX 01. This is a supported mastering profile, not a runtime loading budget.
- Audio may have INDEX 00 at file offset zero, followed by INDEX 01. Indices are
  offsets within each file, not absolute disc LBAs. Their exact values are pinned.
- Audio Lab uses actual two-second silent PCM pregaps and INDEX 01 at `00:02:00`.
  The generic validator pins that content through the contract; it does not assume
  every game needs that gap on every track, or require all pregaps to be silence.

Do not add a virtual PREGAP to an existing recorded pregap: that changes layout.
Do not reorder tracks without updating and testing the game's track references.
A data-only ISO omits external music. Preserve the CUE and every referenced WAV.

Raw `MODE1/2352`, single-file BIN layouts, raw audio, PREGAP/POSTGAP directives,
compressed audio and multi-session layouts are deliberately rejected here.
CUE/BIN is a separate valid delivery choice when required by a target reader;
use an independently verified converter/validator, including sector EDC/ECC,
audio byte order, indices and extracted-payload comparisons. Renaming ISO to BIN
is not a conversion. This helper does not perform or certify that conversion.

## Filesystem and package limits

The ISO reader checks signature, regional block, the current boot layout, volume
and block sizes, both-endian fields, descriptor termination, bounded directories,
ISO level-1 names, duplicate/overlapping extents and exact compiled file contents.
It rejects extensions, interleaving and multi-volume/multi-extent files. This is
not a general-purpose forensic ISO reader or a filesystem conformance suite.

Paths must be relative and portable across macOS/Linux/Windows, without traversal,
case aliases, reserved device names or symlinks. Release ZIPs use stored members,
sorted names, fixed metadata and an embedded `RELEASE.json`. Package verification
checks the exact member list and hashes, then revalidates the disc contract.

The helper's 1 GiB input/package ceiling, 64 KiB manifest/CUE ceiling and 10,000
archive-entry ceiling are resource guardrails, not supported physical CD capacity.
Account for data sectors, PCM sectors, gaps and the chosen medium/reader separately.

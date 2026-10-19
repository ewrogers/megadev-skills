---
name: megadev-release
description: Validate, package and qualify a Megadev Sega CD/Mega CD release, including mixed-mode data/music tracks, provenance and compatibility checks. Use for shipping a disc or diagnosing packaging failures, rather than implementing gameplay or CD runtime services.
---

# Megadev release

Ship the disc layout the game was tested with. A bootable ISO alone does not
prove that its music tracks, loaded modules or release archive are complete.

## Establish the release candidate

Use the project's existing build, content layout, video timing and track numbers.
Pull `ewrogers/megadev:latest`, limit build containers to `--cpus=2`, and record the
actual SDK/compiler and source commits. Retain the original-68000 safeguards.
Keep PAL/NTSC timing separate from US/JP/EU boot/security selection; neither is
inferred from a filename. Do not patch a region tag and claim compatibility.

Read [the supported disc profile](references/disc-profile.md) before running
`scripts/release.py`. Its strict helper targets the current Megadev boot layout,
ISO level-1 data and separate PCM WAV music tracks. Unsupported CUE directives,
raw-sector images and alternate boot layouts need their own verified validator;
do not relabel or silently convert them to pass.

## Validate and package

Follow [the release workflow](references/workflow.md) to bind the disc to compiled
payloads and boot binaries, approve a contract, produce an explicit-file archive,
and verify that archive. Keep the approved contract fixed during later validation.
Generating a new contract from every received artifact is not an integrity check.

The helper checks structural bounds, regional security bytes, file hashes, track
order/indices and PCM format. It does not certify executable behavior, music
quality, a pressed/burned disc, a BIOS or an emulator. Preserve linked RAM limits,
module startup tests and runtime CD/audio ownership checks from their existing
owners; packaging cannot replace them.

## Qualify the delivered artifact

Read [testing and compatibility](references/testing.md) for failure cases, native
ARM64/AMD64 comparisons and emulator/physical-console evidence. Test a fresh unpack
of the actual archive, opening its CUE, rather than a developer build directory.
Check all music tracks, late-loaded areas, return/reload paths and saves without
formatting existing media. Ship only your game, approved notices and instructions;
never add a BIOS, commercial data, toolchain downloads or emulator save files.

Publish only when requested. An unsigned manifest detects accidental changes;
it does not authenticate an untrusted download. Keep a trusted external digest
or use the project's chosen signing/release process when authenticity matters.
The example art, audio and controls are illustrative, not a game-design template.

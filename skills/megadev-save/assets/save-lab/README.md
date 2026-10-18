# Save Lab

A standalone Megadev disc for internal and standard-cartridge Sega CD backup RAM. It writes two private,
versioned BIOS records, verifies readback and can demonstrate recovery from a
damaged newest record. The fields and controls are illustrative. This is not a
raw SRAM driver, a general filesystem or a required game-save format.

## Build and open

Generate a new project with the skill's `scripts/create_project.py`, then:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue save_lab.iso
```

Open `save_lab.cue` in ares with your own configured Mega CD BIOS. On macOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/save_lab.cue"
```

Use an absolute mount path. PowerShell can use `${PWD}`; Docker must run Linux
containers. ARM64 and AMD64 are supported. Unix users can add
`--user "$(id -u):$(id -g)"` for host-owned output. A custom project ID changes
the ISO/CUE name. Use a fresh review copy for `build REGION=EU VIDEO=PAL` or
`build TOUR=1`; never overwrite an image while ares has it open.

## Try it

Typical ares bindings are arrows for the D-pad, J/K/L for A/B/C and Enter for
Start. Use your configured Mega CD mappings.

1. Up/Down selects Level, Coins, Flags or Device; Left/Right changes it.
   Device explicitly chooses Internal or Backup Cart. There is no silent fallback.
2. A saves and verifies it. Change values, then B loads the newest valid record.
3. C resets only the working values, leaving saved files intact. B restores them.
4. Quit/reopen ares normally and load again to check persistent storage rather
   than just the current emulator process or a save-state snapshot.

Counters and field values use hexadecimal. A sequence increments after a verified
write. Main/Sub counters keep advancing. Start shows the recovery instructions.

For corruption recovery, save one set of values, change them, then save again.
With Help visible, press A to damage only the newest Lab record's CRC
and load the older valid record. This refuses to run without two valid records.
The expected status is OLDER RECORD RECOVERED and the earlier values return.
A later normal save replaces the invalid alternate record.

Only `MDSAVELAB_A` and `MDSAVELAB_B` are accessed. The Lab does not format or delete
anything. If MEDIA is not SEGA FORMATTED, use your BIOS memory manager yourself;
formatting existing storage destroys saves. No automatic repair or formatting is
attempted. IO failure can mean insufficient space or another BIOS write/read
problem; it is not silently treated as success. Newer-version and unexpected
size/mode records are preserved instead of overwritten.

## Implementation

- Sub owns internal `_BURAM` calls, a resident 0x640-byte BIOS workspace and 12-byte display
  buffer. Main owns input/UI and one pending request. Parameters precede the token;
  replies retire before reuse. A lost-reply timeout leaves the slot quarantined.
- The BIOS service is synchronous on Sub; Main and Sub INT2 keep their existing
  service ownership. No new ISR or allocator is introduced. This Lab has no
  concurrent runtime CD/audio/ASIC workload to qualify.
- Each normal file is one 64-byte block, plus BIOS filesystem overhead. Both are
  shared-media files, not private reservations of the entire internal RAM.
- Records use explicit big-endian fields, magic, version, length, sequence,
  semantic bounds and CRC-32. Version 1 migrates in RAM with flags=0; an explicit
  save writes version 2 to the other slot and checks all readback bytes.
- The last valid record is retained during replacement. This is record-level
  recovery, not a guarantee against dead batteries or shared filesystem damage.
- The bounded VBlank publisher updates changed opaque text cells. Palettes are
  initialized display-off and remain unchanged during normal display.
- Shared bootstrap/layout code retains `LICENSE.megadev`. ROOM.MMD is only an
  ABI/startup fixture; save records are BIOS files, not Megadev modules.

`save_*` and the record format are project helpers, not Megadev SDK APIs. The
skill references identify actual BIOS services and the optional cartridge path.
The cartridge backend uses Main's `BRAM_CART` vector, its own resident workspace
and the same records on that device. Standard IDs 1-6 require formatted BIOS
media; custom Work RAM handlers are refused. Copying between devices is not
implemented. Ares v148 lacks the cartridge mapping, so select Internal there;
Backup Cart must report unavailable rather than pretend to save.

Compiled Main adapters were also executed with the locally supplied BIOS against
fresh 16/128/512 KiB virtual carts for save/readback/recovery. That checks the
actual BIOS ABI, not physical timing or an interactive cart emulator.

The automatic tour is deliberately read-only: it probes storage and leaves
existing records untouched. Host tests and compiled original-68000 BIOS-model
checks exercise writes, migration and corruption without modifying emulator
files. Manual ares checks cover the actual BIOS and persistence. Physical
consoles, battery failure, full/fragmented media, BIOS variants and cartridge
hardware still require their own qualification. No BIOS or commercial media is
included.

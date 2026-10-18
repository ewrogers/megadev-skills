---
name: megadev-save
description: Implement and diagnose Megadev Sega CD internal and cartridge backup-RAM saves with BIOS file access, explicit versioned records, validation, migration, corruption recovery and non-destructive failure handling; distinguish optional backup cartridges from internal storage.
---

# Megadev saves

Choose what the user's game needs to persist. Keep checkpoints, options and
temporary scene state distinct; the Lab's three fields are only examples.

- Read [Backup RAM and devices](references/devices.md) before choosing a BIOS
  interface or making claims about the optional cartridge.
- Read [Records and recovery](references/records.md) for byte layouts, migration,
  redundant records, readback and failure policy.
- Read [Validation](references/testing.md) for compiled BIOS-ABI tests, emulator
  persistence checks and the remaining physical-hardware evidence.

Essential contracts:

- Internal backup RAM is shared, BIOS-managed storage, not game-cartridge SRAM
  at SGDK offsets. Sub owns the `_BURAM` service; serialize its calls.
- Initialize/probe first. Missing, unformatted or other-format media is a status
  to report, not permission to format. Do not erase unrelated files to make room.
  Offer formatting only as a separate explicitly confirmed whole-device action,
  naming the device and defaulting to Cancel; use BIOS BRMFORMAT and re-probe.
- Use project-specific names and validate file size/mode before a BIOS read into
  a bounded buffer. Do not deserialize by casting raw bytes to a C struct.
- Validate magic, version, payload length, checksum and semantic ranges before
  changing live game state. Preserve valid future-version data.
- Write a replacement record without destroying the last verified good one;
  read back before accepting success. A write failure must remain visible.
- Keep BIOS work buffers and requests alive until completion. A lost reply does
  not authorize another write or reuse of its request slot.
- Cartridge saves use the distinct Main-side `BRAM_CART` path. Require a supported
  standard ID and successful BIOS initialization; refuse custom Work RAM handlers.

Use `ewrogers/megadev:latest` and inspect its selected source before teaching an
API. The [standalone Save Lab](assets/save-lab/README.md) exercises internal and cartridge
BIOS files, versioned records and explicitly selected recovery and whole-device formatting demonstrations.
Its `save_*` helpers and record format are project code, not Megadev APIs.
Build/test containers use at most two CPUs.

Reuse the game's existing system, memory and scene owners. This skill owns save
records and persistence policy; it does not replace input, allocation, scene flow
or audio/CD arbitration. It prescribes no art style, controls or progression.

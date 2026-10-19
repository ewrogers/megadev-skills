---
name: megadev-compression
description: Choose and validate compression for Megadev Sega CD assets, comparing sector-rounded disc size, decode time and RAM, with safe Kosinski decoding and loading/preloading integration.
---

# Megadev compression

Choose compression per asset and loading path. A smaller file can still cost
more sectors, more working RAM or more time before an asset is usable. Preserve
the game's asset formats, visual design and loading behavior.

- Read [Formats and authoring](references/formats.md) for audited Megadev/BIOS
  support, a deterministic Kosinski encoder and input/output validation.
- Read [Loading and budgets](references/loading.md) for ownership, sector-sized
  staging, peak RAM, Sub decoding and preloading trade-offs.
- Read [Measurement and validation](references/testing.md) for real decoder
  execution, timing limits, ARM64/AMD64 comparison and PAL/NTSC auditions.

Use `ewrogers/megadev:latest`; inspect its actual decoder and ABI before teaching
calls. The audited SDK revision and primary links are in the references. Do not
port SGDK's resource compiler, `unpack` calls or codec names as Megadev APIs.

Essential contracts:

- Bound source consumption, decoded length and history references **before**
  calling a decoder that has no bounds arguments. Keep that validated source
  immutable until decoding completes. Validate the decoded asset before use.
- Round CD reads to 2048-byte sectors and reserve that full writable range.
  Keep encoded input and output disjoint, with exclusive CPU ownership.
- A request acknowledgement, read completion, decode completion and visible
  upload completion are different events. Cancellation/timeouts do not free
  buffers from a live reader or decoder.
- Keep decoding out of VBlank/interrupt handlers. Main/Sub separation can keep
  gameplay responsive, but does not make decode cost or shared-bus work free.
- Report stored bytes, sectors, decoded bytes, active buffer usage and reserved
  capacity separately. Compression does not shrink the final VRAM requirements.

The self-contained [Compression Lab](assets/compression-lab/README.md) compares
raw versus Kosinski artwork and an explicitly labeled random-noise sample.
`scripts/create_project.py` generates a new project without replacing files.
Its encoder, validator, measurement helpers and protocol are application code;
`Kos_Decomp` is the actual SDK assembly decoder. Both raw and packed versions
are included for comparison, not as a recommended shipping layout.

Reuse the game's existing reader, clock, memory leases and renderer.
`megadev-streaming` owns loading mechanics, `megadev-memory` covers lifetimes,
and `megadev-performance` covers broader profiling. This skill adds compression
choice, validation and decode integration; it prescribes no art style or game flow.

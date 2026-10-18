---
name: megadev-streaming
description: Plan and implement safe CD loading, preloading, resident art and bounded uploads in Megadev Sega CD games, including delayed reads, cancellation and Word RAM handoffs.
---

# Megadev streaming

Keep play independent of disc latency. Load into an owned staging buffer, validate
complete data, then publish it through the game's existing graphics service.
Use the current game's controls, art and architecture. Streaming Lab is an
illustration, not a game-design template.

- For the actual loader, read [CD access](references/cd-access.md): audited
  Megadev symbols, sector rounding, directory checks and interrupt service.
- For residency, preload triggers and transfer budgets, read
  [Scheduling](references/scheduling.md).
- For failure handling and verification, read [Testing](references/testing.md).

Use `ewrogers/megadev:latest`; inspect its source and record the revision before
assuming an API. Do not substitute SGDK resources, MAP/DMA/TSK calls or a
cartridge bank-switching design. Streaming is separate from decompression.

The critical contracts:

- One owner of the CD reader and its global request fields. Publish filename and
  destination before `access_op`, and keep the destination until the reader is idle.
- A timeout stops consumption; it does not prove the drive stopped writing.
  Drain and retire a late read before reusing its buffer. An unconfirmed ownership
  handoff stays unavailable; never forge completion by clearing the reader's state.
- File bytes, sector-rounded read bytes, decoded bytes and VRAM uploads have
  separate limits. Validate directory size before starting a transfer.
- Main can keep rendering resident VRAM art while Sub owns 2M Word RAM. Main
  must not read that RAM until ownership and the matching result are confirmed.
- Queued transfers either own/pin their source or copy it into resident staging.
  Do not hand Word RAM away beneath a VBlank upload.

`megadev-memory` covers RAM lifetimes and 1M/2M views; `megadev-system` covers
command protocols; `megadev-graphics` covers the VDP publisher. Reuse those
contracts when available, without requiring a particular framework. CD-DA and
PCM coexistence need a separate audio policy and measurements.

To examine a working example, generate the standalone
[Streaming Lab](assets/streaming-lab/README.md) with
`scripts/create_project.py`. Its `stream_*` helpers and MDA1 format are project
code, not upstream APIs. Build/test containers use at most two CPUs.

---
name: megadev-scaling
description: Build and diagnose Sega CD graphics ASIC rotation, scaling and perspective floors in Megadev, covering stamp maps, per-line trace vectors, 2M Word RAM ownership, completion and bounded VDP uploads. Use for the ASIC path rather than ordinary VDP sprite animation or software-only 3D.
---

# Megadev scaling

Decide whether the asset and effect benefit from an ASIC-generated image. The
ASIC samples a stamp map into Word RAM; it does not give a VDP sprite a rotation
flag, add palette lines or display the result by itself.

- Read [ASIC layouts and projection](references/asic.md) before preparing stamps,
  trace lines or register values. Read the selected Megadev headers rather than
  inventing convenience APIs or importing SGDK types.
- Read [Ownership and publication](references/pipeline.md) for Main/Sub leases,
  completion, cached snapshots, hidden VRAM uploads and cost accounting. Its
  performance section explains the measured 5→30 image/s improvements and how
  to profile copying, HUD work and handoff delays without weakening correctness.
- Read [Validation](references/testing.md) for math, compiled adapters, ares and
  checks that still need a physical console.

Essential contracts:

- The graphics operation requires 2M Word RAM and Sub ownership. Build immutable
  source/map/trace inputs before writing the start register. No CPU modifies them
  or hands away Word RAM while the ASIC is active.
- Wait for hardware completion, then confirm the ownership handoff. A matching
  mailbox reply alone does not authorize a Main read. Timeouts report failure;
  they do not cancel hardware or make a pending buffer safe to reuse.
- Validate alignments, region extents, image dimensions, signed step ranges and
  the selected stamp/map layout. Keep output separate from source/map/trace.
- Treat the inverse projection and its fixed-point units explicitly. Floor
  perspective uses a different start/step per output line; this is an image
  sampler, not a polygon engine, depth buffer or general camera system.
- Budget trace generation, ASIC work, Main copying, VRAM residency and VBlank
  transfers separately. Publish an image only after every staged chunk completes.
  Extra RAM/VRAM can avoid visible tearing, but they are not free.
- Share the game's existing memory, Main/Sub, renderer and interrupt owners.
  This skill does not introduce a competing CD, audio, allocator or input system.

The [standalone Scaling Lab](assets/scaling-lab/README.md) demonstrates zoom,
rotation and a repeating perspective floor with original native-resolution art.
Its `scale_*` helpers and fixed profile are project code, not Megadev APIs. The
mosaic, colors, controls and layout are illustrative, not a game-design template.

Use `ewrogers/megadev:latest`; pull it before validation and inspect its SDK source.
Limit Docker containers to two CPUs. Record actual compiler/SDK revisions and
separate compiled model evidence from emulator and real-hardware observations.

# Megadev integration and validation

The Lab uses actual `main/bios.h`, `main/vdp.h`, `main/memmap.h` and `system.h`
contracts from [Megadev 7a7246c](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib).
Reinspect the selected `ewrogers/megadev:latest` before changing assembly/BIOS
assumptions. `iso_*`, the sorter, surfaces and packet format are application code.
No SGDK engine, resource compiler or ASIC transformation is required.

## Residency and publication

Native art has 368 sprite patterns, 272 deduplicated background patterns, a
64×32 map, 96 font patterns and four palettes. Initial VRAM regions are explicit
and checked against BIOS tables. Tall art is packed per hardware piece. All
palettes load with display off; normal frames never write CRAM.

Uncompressed art plus code/fixtures would overfill the chosen Main RAM partition.
The example stores bounded literal/repeat packets, decodes each asset once into
the first 16 KiB of Main-owned 2M Word RAM, and uploads it before gameplay. The
boot loader has finished relocating Main and Sub has granted Word RAM; no other
job uses it in this Lab. This scratch policy must be negotiated with the existing
memory/system owners when adapting, not copied into a running stream or ASIC job.

Use `BIOS_DMA_XFER_WORD_RAM` for those Word RAM uploads: unlike the ordinary RAM
transfer, it handles the hardware's final-word DMA quirk. Register-preserving C
adapters keep the BIOS call ABI explicit. Word RAM ownership and DMA completion
must hold until the transfer finishes. The packet decoder rejects truncation,
overflow and non-exact output; its output can be partial on failure and must not
be published. This is a small private art packer, not a new general codec policy.

One Main VBlank path uploads immutable SAT/HUD/camera snapshots, then triggers
`BIOS_TRIGGER_IFL2`, reads `BIOS_READ_JOYPAD` and latches edges. It validates frame
index/count and early VBlank; late work remains pending. Plane B scroll and sprite
positions therefore use the same committed camera, with a fixed Window HUD.
Audio is one Lab-only finite PSG cue/music owner; adapt events into an existing
game's sound policy instead of competing with its drivers.

## Evidence

- Host O0/O3 contracts: floor/inverse projection, elevation ordering, stable IDs,
  support overrides, cycles/malformed input, cached/full equivalence, stairs,
  underpass, upper edges, blockers, pause and PAL/NTSC equality.
- Independent convex hull/SAT oracle for 6,000 projected-box pairs; independent
  DAG ordering checks; complete raw ground reconstruction through packed tiles.
- Original-M68000 execution of contracts, exact bounded art decode, immutable
  publication, camera writes, late-frame deferral, malformed index/count rejection
  and authored tour views without scanline admission rejection.
- Fresh ARM64/AMD64 US/JP/EU release, US debug and qualification matrices, with
  complete target-payload comparison. Standalone install tests disable telemetry.

In ares, test walking behind towers/trees, ascending/crossing/descending the bridge,
walking underneath, cutaway/info, animation, music and pause in PAL and NTSC.
CPU tests model registers and do not qualify drawn pixels, console cycles, analog
audio, drive latency or physical-controller behavior. Real hardware still needs
acceptance, including the Word RAM DMA path and worst combined rendering workload.

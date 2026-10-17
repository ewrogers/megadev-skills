# Megadev source audit

Research date: 2026-10-17. This is an adaptation baseline, not a claim that a
Megadev skill or game has been implemented or tested.

Implementation follow-up: `megadev-project` now exists. The toolchain safeguard
PR is merged and fresh ARM64/AMD64 images pass the Project Lab matrix; see
[validation evidence](validation.md). The audit below records the source-reading
baseline separately from those later tests.

## Source and toolchain baseline

Inspected the Megadev source bundled in the locally available
`ewrogers/megadev:latest` image and the packaging in `../megadev-docker`.
The source lock pins upstream to
[`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef).
This audit did not pull a fresh image or compare upstream HEAD. Future skill
implementation should check the installed source and rerun both architecture
suites rather than assuming that this report proves a newer image works.

Our Docker lock selects GCC 16.2.0 and binutils 2.47. The container includes
GCC, binutils, GDB, Megadev and its examples, and `mkisofs` from genisoimage.
The public usage image remains `ewrogers/megadev:latest`; record actual versions
and source revisions in validation evidence rather than putting image digests
into user-facing commands.

## What Megadev supplies

Megadev is a lower-level Sega CD development kit with assembly/C headers,
BIOS wrappers, module/linker conventions, examples and disc build rules. It is
not an SGDK API compatibility layer. Source reuse must distinguish portable
game logic from SDK-dependent rendering, audio, input, allocation and startup.

The [manual](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/manual.md),
[program design guide](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/program_design.md)
and [module guide](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/modules.md)
are the starting points. The bundled upstream examples are `hello_world`,
`gfx`, `cart`, `bram`, `pcm_playback` and `transforms`. They are upstream
examples, not Labs written for our skill repository.

| Area | Verified source support | Adaptation consequence |
| --- | --- | --- |
| Project/build | `megadev.make`, `new_project/`, linker scripts under `cfg/`, IP/SP bootstrap and MMD/SMD modules | Build a new standalone project workflow; do not copy SGDK Makefiles or ROM headers. |
| CPU communication | Main/Sub gate-array headers, communication registers, `wait_2m()` and `grant_2m()` | Teach ownership and acknowledgement protocols before asynchronous loading or parallel jobs. |
| Graphics | Main BIOS VDP, palette, DMA, map and sprite-list helpers; low-level VDP headers | Shared VDP limits apply, but SGDK's sprite engine and resource APIs do not transfer. |
| Input | `init_joypads()`, `read_input_joypad()` and optional Main BIOS controller functions | Basic pad support has concrete examples; extended devices need dedicated research and tests. |
| Audio | Sub BIOS disc-audio controls and PCM register/configuration helpers; PCM playback example | Plan separate FM/PSG/Z80, Sega CD PCM and CD-DA ownership; no supplied XGM/XGM2 runtime should be assumed. |
| Disc I/O | CD-ROM access wrapper, file lookup, read/transfer operations and module-loading example | Stream from a disc/filesystem with bounded buffers and seek-aware design; cartridge far-data APIs do not apply. |
| Saves | Sub BIOS BRAM wrappers and a directory/read/write example | Reuse versioning and corruption-recovery concepts, replace SRAM storage with BRAM file semantics. |
| ASIC | `transforms` example, stamp/image/trace-table gate-array registers, `xform.h` fixed-format macros | A specific scaling/rotation skill is justified, but it must teach the actual render pipeline rather than promise a generic sprite transform API. |
| Compression | Kosinski support and BIOS graphics decompression are present | Recheck codec formats, destinations and register/stack contracts; SGDK APLIB/LZ4W wrappers are not portable APIs. |

## System architecture that every advanced skill must respect

The Main CPU owns the Mega Drive VDP/VRAM interface; the Sub CPU handles CD
drive and internal backup-RAM services. Both CPUs are original 68000s.
The program-design guide describes 512 KiB PRG RAM, with 24 KiB reserved for
the Sub BIOS, and 256 KiB Word RAM shared between CPUs. Those are memory-map
planning constraints, not an application allocator's available-byte report.

Word RAM's 2M mode gives its contents to one CPU at a time. Its 1M mode splits
it into separately owned halves with bank exchange. Code can execute from
Word RAM, but handing that memory to the other CPU then has execution and
interrupt-handler consequences. An interrupt routine or persistent kernel
must not depend on inaccessible/transient module memory. The recommended
lesson is explicit residency, ownership and lifetime accounting, not a direct
copy of SGDK's cartridge bank switching or user-task scheduler.

Main BIOS use is optional; Sub BIOS use is part of the normal CD architecture.
Choosing Main BIOS services also adopts their memory/work-buffer conventions.
Do not mix direct hardware writes, BIOS-owned tables and a separate engine
without documenting who updates each resource.

The [C development guide](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/dev_in_c.md)
specifically warns about missing hosted-C facilities, data/BSS initialization,
stack size, ABI/register conventions and floating point. `ipx_init.s` or
`std_init.s` provides the intended initialization machinery when included
correctly. Do not teach that an initialized global is automatically ready in
every module layout. The Main BIOS's small default stack also needs an actual
application budget. A custom libc/allocator is a project decision, not an
existing Megadev equivalent to SGDK's memory API.

## APIs and examples worth anchoring new work to

- [Main BIOS wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h):
  `bios_read_joypad`, `bios_detect_controller`, `bios_load_map`,
  `bios_copy_sprlist`, `bios_dma_xfer`, `bios_dma_xfer_word_ram`,
  `bios_dma_queue`, palette loads/fades, and font/text helpers. Their buffer
  contracts and VBlank service must be checked per use; a similarly named SGDK
  function is not evidence of equivalent behavior.
- [Direct input helper](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.h):
  the inspected polling sequence collects ordinary three-button pad bits.
  The BIOS header has MegaMouse and MultiTap controller-type constants, but
  constants/detection alone do not prove usable six-button, mouse or multitap
  reporting. Do not advertise parity with `sgdk-input` without executable
  coverage and emulator/hardware checks.
- [CD access guide](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/cdrom.md)
  and [header](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/cdrom.h):
  filename, destination, operation and completion state are explicit. The
  documented convenience loader is `load_file_sub`; the C lookup helper is
  `find_file_c`. The skill should distinguish a convenience wait from a
  gameplay-safe asynchronous transaction.
- [Disc format guide](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/disc.md):
  an ISO is the data track, not a complete mixed-mode audio-disc description.
  CD-DA tracks require a coherent CUE layout; do not merely put WAV files in
  the ISO and claim they are CD audio. Validate track offsets and pregap rules
  against the produced image and player, not a copied example command alone.
- [PCM example](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/examples/pcm_playback):
  Sub CPU PCM playback includes explicit sample loading and channel activity.
  Helpers include `pcm_clear_ram_c` and `pcm_config_channel_c`; this is not a
  complete general sound mixer or music-authoring environment.
- [BRAM wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bram.h)
  expose init/status/search/read/write/delete/directory/format/verify services.
  The [example](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/examples/bram)
  expects formatted BRAM. Formatting must be an explicit destructive action,
  never an automatic recovery step that erases somebody's other saves.
- [Transform example](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/examples/transforms):
  its `spx.c` builds trace-table/render-register state; `ipx.c` coordinates
  display and Word RAM transfer. `lib/xform.h` supplies coordinate-format
  typedefs/macros rather than a high-level `scale_sprite()` API. Include the
  output buffer, transfer cost and CPU ownership when showing ASIC gains.

## Asset pipeline and compiler gaps to resolve first

The inspected upstream image did not contain an SGDK-style ResComp pipeline.
Examples ship preconverted `.chr`, `.pal`, map, stamp and sprite-definition
data and include it through assembly resources. The graphics example points
to separate `scdtools` utilities for conversion. Establish a reproducible,
licensed conversion pipeline before porting the SGDK asset Labs; do not
assume PNG/WAV imports work simply because files are in a resource directory.

The current Docker compiler workaround forces original-68000 defaults,
GNU C17 unless overridden, and `-fno-auto-inc-dec` unless explicitly opted
back in. Assembler CPU-option guards and executed machine-code regressions
exist in `megadev-docker`. However, the inspected workaround and Megadev build
defaults do **not** add SGDK Docker's later `-fno-strict-aliasing` default.
Review this difference with an executed regression before claiming identical
safeguards. This finding is not itself proof of an existing miscompile.

Do not replace upstream project optimization flags wholesale. The template
starts with conservative project flags, and inline assembly/BIOS ABI behavior
needs release-build testing under the selected compiler. A successful compile
does not validate interrupt safety, disc latency, DMA timing, sound quality,
extended peripherals or real hardware behavior.

## Practical skill-family implications

Portable mechanics such as collision, AI, gameplay, puzzle logic, dialogue
state, RPG rules and deterministic models are conceptually reusable. Their
draw/input/audio/save adapters need Megadev implementations and fresh tests.
For the first releases, reference those concepts selectively rather than
bulk-copying every genre skill and Lab into another repository.

The first distinct Megadev foundations should cover project/disc boot,
CPU/memory/BIOS ownership, native asset conversion and VDP rendering. Audio,
CD streaming, BRAM saves and ASIC transforms then have clear prerequisites.
Release/debug/testing need disc and multi-CPU coverage; hardware validation
must remain separate from ares and CPU-emulator evidence.

The strongest Sega CD-specific opportunities are dual-CPU scheduling,
Word RAM exchange, CD seek/preload management, PCM/CD-DA coexistence, BRAM
files and ASIC rendering. These are substantial new subjects rather than
find-and-replace versions of SGDK APIs.

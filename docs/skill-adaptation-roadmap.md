# Megadev skill adaptation roadmap

Audit date: 2026-10-17. This is a proposed roadmap, not a claim that the skills
or their examples have already been ported. The starting inventory is all 38
skills in `ewrogers/sgdk-skills`; every one is mapped below.

## Recommendation

Build a Megadev-native foundation first. Reuse the proven explanations, portable
algorithms, failure cases and test methodology. Reimplement platform integration
against the selected `ewrogers/megadev:latest` source. Do not bulk-copy the SGDK
skills or require their installation to develop with Megadev.

The add-on shares the Mega Drive VDP and its rendering constraints. It changes
where code/data live, how content arrives, how CPUs cooperate and which sound
and save facilities are used. More disc space is not more VRAM or unlimited main
CPU time. See [source audit](megadev-source-audit.md) for pinned source evidence
and unverified areas.

## Toolchain prerequisite

Megadev Docker PR #2 is merged and `ewrogers/megadev:latest` includes the
`-fno-strict-aliasing` compatibility default and executed O2/LTO regressions.
Fresh ARM64 and AMD64 images were pulled for Project Lab; all four build cases
produce identical target payloads across architectures. Keep base-68000
restrictions and byte-copy regressions as future toolchain gates. The aliasing
negative control deliberately violates normal C aliasing rules; valid byte
access is tested separately. See [validation](validation.md).

## Delivery stages

The user requested this order: **foundations → systems → advanced → debugging /
testing → release**. These are teaching and inventory groups. Every implementation
still needs meaningful tests and documented hardware limits from its first PR;
we do not defer QA until the debugging/testing group is written.

### 1. Foundations

Start with `megadev-project`, then `megadev-hardware`, `megadev-assets`,
`megadev-graphics` and `megadev-input`. Add planning, C best practices and math
as the foundation becomes executable. PixelLab integration is optional asset
production guidance, not a requirement to use generated art.

- Project: container commands, boot/IP/SP/module layout, C startup, stacks,
  regional builds and a minimal known-good main/sub application.
- Hardware: the base VDP/CPU/Z80 remain relevant, plus the Mega CD main/sub
  architecture, Gate Array, memory visibility, PCM and graphics ASIC. Distinguish
  fact, BIOS service and an optional software convention from the start.
- Assets/graphics: actual Megadev converters and formats, palette/tile/sprite
  budgets, font loading, and one foreground owner for VDP/transfer service.
- Input: verify the paths we teach on actual examples. A mouse/multitap constant
  in a header does not establish a complete tested driver.

**First skill PR: `megadev-project`.** Include the minimum hardware explanation
needed to understand booting and CPU roles, with links/boundaries for the deeper
hardware/system skills. Prove a small original Lab, not a renamed SGDK scaffold.

### 2. Systems

Build `megadev-system`, `megadev-memory`, `megadev-streaming`, `megadev-audio`,
`megadev-scenes` and `megadev-save`. Then add gameplay, collision, animation,
levels, UI/dialogue/localization and multiplayer patterns as needed.

This group is specifically about Mega CD integration, not just adding CD reads:
main/sub command handshakes and BIOS responsibilities; Program RAM and Word RAM
visibility/ownership including 1M/2M modes; stacks and module residency; PCM RAM,
channel/sample setup and CD-DA; backup-RAM capacity/failure/recovery; and disc seek,
read, buffering and preload deadlines. Verify each operational path against the
selected source and hardware documentation before teaching it.

Do not treat the sub-CPU as an unconditionally free worker. Budget its platform
services and sound/disc responsibilities alongside any game computation.

### 3. Advanced

Prioritize **`megadev-scaling`** as a dedicated hardware skill: ASIC stamps,
trace tables, rotation/scaling, supported layouts, operation completion, Word RAM
ownership and the cost of getting the result to the VDP. It is not a sprite flag
or an unlimited accelerator.

Follow with compression, raster effects, bounded particles/effects, isometric
rendering, AI/combat and genre-specific patterns. Genre examples stay illustrative;
they never prescribe art, music, controls, progression or game structure. Ordinary
same-CPU coroutines, if useful, are a separate topic from main/sub coordination;
SGDK's TSK API is not assumed to exist.

### 4. Debugging and testing

Expand dedicated `megadev-debug`, `megadev-testing` and `megadev-performance`.
Cover both CPU symbol/module views, command/ownership tracing, deterministic
models/replay, failures and timeouts, PAL/NTSC, CD latency and audio contention.
Publish a precise emulator-versus-console validation checklist. A base-68000
execution harness does not simulate the Gate Array, drive, ASIC, PCM or full
interrupt timing merely because compiled arithmetic passes.

### 5. Release

Finish `megadev-release`: boot/region and module validation, data versus mixed-mode
disc layouts, audio track details, reproducible payload accounting, licenses and
packaging, plus a real-hardware acceptance checklist where equipment is available.

All names use the `megadev-` prefix to match the separate repository and toolchain.
The sequence organizes the skills; it does not require all of them in every game.

## Responsibility seams

| Owner | Responsibility | Hand-off |
| --- | --- | --- |
| hardware | Explain hardware behavior, limits and register/BIOS distinctions | Other skills cite these facts rather than inventing new limits. |
| memory | Address visibility, allocation lifetimes, buffers and Word RAM ownership | System/streaming/scaling obtain and release resources through that policy. |
| system | Main/sub command protocol, task state, acknowledgement and deadlines | No second renderer, audio policy or allocator. |
| graphics | VDP rendering and foreground transfer/frame-service ownership | Consumers request uploads; they do not install competing frame loops. |
| streaming | Disc work, cache/residency, bounded load/decode/upload scheduling | Uses system, memory and graphics contracts. |
| audio | Sound drivers, CD-DA/PCM/FM/PSG and sound request ownership | Coordinates drive/RAM needs without silently taking them over. |
| scaling | ASIC input/output layouts and operation lifetime | Acquires Word RAM safely; completed art goes through the graphics owner. |
| scenes | Game flow and resource lifetime between scenes/modules | Coordinates these systems; does not replace their underlying policies. |

## Complete inventory mapping

**Adapt** means the conceptual implementation is substantially reusable, with
platform integration rewritten. **Rewrite** means the platform-facing workflow
needs a new implementation. **Replace** means no direct equivalent should be
created. None means the existing SGDK code is a drop-in Megadev dependency.

| Existing skill | Approach | Proposed Megadev home | What changes / stays |
| --- | --- | --- | --- |
| sgdk-project | Rewrite | project | Disc boot, initial/sub programs, linker/build layout and the Megadev container replace the cartridge/SGDK scaffold. |
| sgdk-planning | Adapt | planning | Reuse budget-first planning; add main/sub CPU, CD latency, Word/Program/PCM RAM and residency budgets. |
| sgdk-best-practices | Adapt | best-practices | Reuse sound C17/68000 practices; audit Megadev headers, calling conventions, ownership and interrupt assumptions. |
| sgdk-assets | Rewrite | assets | Keep native pixels/palette validation; replace ResComp/.res resources with verified Megadev converters, binary formats and disc files. |
| sgdk-pixellab | Adapt | pixellab | Prompting/native-resolution/palette studies transfer. Export for Megadev, and treat ASIC-friendly layouts as an optional target, not a universal requirement. |
| sgdk-graphics | Rewrite | graphics | VDP tiles, planes, palettes, sprite and scanline limits remain. Replace SGDK sprite/VDP/DMA APIs and implicit resource management. |
| sgdk-animation | Adapt | animation | Timelines, events, interruption, hitbox timing and one-hit contracts transfer; use a Megadev renderer and game clock. |
| sgdk-effects | Adapt | effects | Bounded pools, priorities and particle selection transfer; remeasure submission/DMA costs in the new renderer. |
| sgdk-raster | Rewrite | raster | Same base VDP effects; re-audit BIOS/interrupt handlers, palette timing, DMA and coexistence with sub-CPU work. |
| sgdk-input | Rewrite | input | Control semantics transfer. Verify BIOS/direct input and each pad/mouse/multitap path rather than copying JOY APIs or promising unsupported devices. |
| sgdk-multiplayer | Adapt | multiplayer | Joining, shared cameras and independent respawns transfer after input verification. Two CPUs do not imply one CPU per player. |
| sgdk-ui | Adapt | ui | Layout, nine-slice panels, menus, glyph/tile budgets and HUD ownership transfer with a new text/graphics backend. |
| sgdk-localization | Adapt | localization | Message IDs, glyph coverage, translated layout tests and paging transfer; rebuild font loading and text APIs. |
| sgdk-dialogue | Adapt | dialogue | Branching, choices, portraits and reveal timing transfer; CD voice is an optional audio/streaming integration. |
| sgdk-levels | Adapt | levels | Map/collision separation and camera models transfer; rewrite Tiled output/runtime integration that assumes SGDK MAP resources. |
| sgdk-isometric | Adapt | isometric | Projection, sorting, elevation and occlusion transfer. ASIC scaling is optional and does not solve depth sorting. |
| sgdk-streaming | Rewrite | streaming | Make this CD-native: seek/read latency, sector/file layout, RAM staging, decode/upload deadlines and reusable resident art. |
| sgdk-compression | Rewrite | compression | ROM/CPU/RAM trade-off methodology transfers; verify actual Megadev codecs and distinguish packed file size, disc layout and decode cost. |
| sgdk-gameplay | Adapt | gameplay | Timing, movement, collision, cameras and bounded object pools transfer; replace framework calls/types. |
| sgdk-collision | Adapt | collision | Sweeps, rays, sliding and executable geometry contracts largely transfer as portable C; recheck fixed-point/overflow conventions. |
| sgdk-combat | Adapt | combat | Attack windows, guard rules, damage/knockback and optional co-op contracts transfer; art/tuning stay illustrative. |
| sgdk-math | Adapt | math | Integer/fixed-point reasoning and geometry transfer; do not assume SGDK fix16/fix32, lookup tables or math function names. |
| sgdk-ai | Adapt | ai | States, sensing and bounded pathfinding transfer; sub-CPU offload is a separately measured option with snapshot/result ownership. |
| sgdk-scenes | Rewrite | scenes | Game-flow patterns transfer, but scene lifetime must account for module loading, disc operations, shared RAM and audio handoffs. |
| sgdk-audio | Rewrite | audio | Keep authoring/listening-test principles. Rebuild runtime guidance around CD-DA, Sega CD PCM and verified FM/PSG drivers, not SGDK XGM APIs. |
| sgdk-save | Rewrite | save | Versioning, validation and recovery transfer; use BIOS backup-RAM file operations/capacity/errors instead of copying cartridge SRAM access. |
| sgdk-cartridge | Replace | streaming + system | Do not create a Sega CD mapper/banked-ROM clone. Transfer resident/shared-content planning into disc loading and memory ownership; cartridge/hybrid targets are separate scope. |
| sgdk-testing | Adapt | testing | Seeded replay and deterministic game-model checks transfer. Add CD failure/latency, CPU handshake and buffer ownership tests; rework ROM packaging/emulator runners. |
| sgdk-hardware | Rewrite | hardware | Retain base-console limits; add BIOS/drive/RAM/PCM/ASIC checks and distinguish emulator evidence from physical Sega CD behavior. |
| sgdk-debug | Rewrite | debug | Keep evidence-first diagnosis. Handle separate main/sub symbols, loaded module addresses, communication state and drive/BIOS failures. |
| sgdk-performance | Rewrite | performance | Separate main/sub CPU time, transfers, RAM and deadlines. Extra CPU capacity does not remove VDP/scanline/DMA constraints. |
| sgdk-memory | Rewrite | memory | Account for CPU visibility, Program/Word/PCM RAM, buffer ownership and module lifetimes. System/streaming/scaling follow this ownership contract. |
| sgdk-multitasking | Replace | system | Do not port SGDK TSK APIs or its supervisor/user setup. Teach verified main/sub coordination and bounded jobs; same-CPU coroutine work is a distinct optional topic. |
| sgdk-release | Rewrite | release | Validate disc image/layout, boot/region metadata and any audio tracks/licenses; cartridge ROM padding/checksum rules are not disc release validation. |
| sgdk-platformer | Adapt later | platformer | Movement/enemy/powerup patterns transfer after core platform skills. No copied Lab world, controls, art or tuning. |
| sgdk-shmup | Adapt later | shmup | Waves, patterns, scoring and pools transfer; renderer/audio/streaming integration must be Megadev-native. |
| sgdk-rpg | Adapt later | rpg | Stats, turn resolution, inventory, quests and save contracts transfer. CD capacity is not permission to keep all assets resident. |
| sgdk-puzzle | Adapt later | puzzle | Grid transactions, undo, falling-block timing and rule tests transfer with the new input/rendering backend. |

## Sega CD topics that must be explicit

- **CPU/memory ownership:** design bounded main/sub work with command/result
  handshakes, stable inputs, acknowledged completion and safe buffer reuse. The
  sub-CPU also services platform work; it is not an unconditionally free worker.
- **C runtime/startup:** verify initialized data/BSS setup, stacks, module entry
  and available library routines. Do not assume SGDK or standard-library helper
  functions exist just because the source is written in C.
- **Disc behavior:** distinguish file/sector reads, buffering, seek latency and
  preloading from the final VDP upload. Plan for late/failed work without making
  gameplay dependent on an ideal emulator drive.
- **Sound resource ownership:** CD music, PCM sample RAM and FM/PSG are different
  paths. Confirm driver availability and concurrent CD/data behavior with tests.
- **Graphics ASIC:** rotation/scaling requires its own data layout, ownership,
  operation completion and display-transfer guidance; it is not a VDP sprite flag.
- **Backup RAM:** capacity, existing user data, write failures and recovery matter.
  Never auto-format storage as a response to an ordinary failed load.

## Acceptance bar for each port

- Verify APIs and tool syntax against the container's pinned upstream source;
  cite source locations and record the image/tool versions actually tested.
- Carry over useful portable-C model tests, then execute compiled base-68000 code
  for both CPU targets where applicable. Match artifacts on ARM64/AMD64.
- Add platform failure tests: stale commands, wrong RAM ownership, incomplete
  output, delayed/failed reads and unavailable save capacity as applicable.
- Build a small standalone Lab and verify relevant PAL/NTSC behavior in ares.
  Label the remaining physical-console/drive/audio/timing checks explicitly.
- Keep each installed skill self-contained. Related skills explain boundaries,
  not a mandatory all-skills framework. Preserve the user's design and controls;
  Lab art/music/progression are illustrative only.

## Sources and scope

- [Megadev upstream](https://github.com/drojaazu/megadev) and its pinned source
  audit linked above establish framework behavior, not similarity of API names.
- [Our Megadev toolchain](https://github.com/ewrogers/megadev-docker) supplies the
  container/architecture/build validation baseline.
- [SGDK skill inventory](https://github.com/ewrogers/sgdk-skills/tree/main/skills)
  and [groups](https://github.com/ewrogers/sgdk-skills/blob/main/skills.sh.json)
  establish the 38 source skills. Proposed names/grouping/order are recommendations.

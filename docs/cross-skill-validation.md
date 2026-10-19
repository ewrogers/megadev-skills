# Cross-skill validation: Glasshouse After Hours

## Brief and scope

Build an original, quiet exploration game: an evening gardener restores a
moonflower's water supply across a conservatory and a rainworks room. It has
readable interaction prompts, a caretaker conversation, a valve choice, field
notes, pause/save, Continue and a small ending. No combat, collectible currency,
beacon race or Lab progression is carried into its design.

The game is in `validation/glasshouse`, created independently through
`validation/create_project.py`. It is a repository integration fixture, not a
26th skill. The registry retains 25 skills; this milestone tests their seams.
New native art, character frames, palettes and music are procedural and licensed
with the example. Existing skills remain neutral about style, mood and controls.

## Ownership map

| Concern | Reused owner | New application responsibility |
| --- | --- | --- |
| Main/Sub startup | project boot/module shell | Build ID and original-game entry |
| Scene transitions | `scene.c` leases/generations | Enter/leave/restore intent |
| Disc loading | `stream_*` ownership and bounded validation | Two room caches, preload trigger |
| Drive/music/PCM | `audio_*` arbitration | Two tracks and finite interaction cues |
| Panels and page publication | `ui.c`, `paint.c` | Original menu/dialogue/HUD composition |
| Text layout | `d_compile` from dialogue | Original words, reveal/choice controller |
| Save commit/CRC/recovery | checkpoint adapters and save codec | Private record names and semantic payload validation |
| Game time | one 300-unit clock adapter | Bounded 60-Hz movement and paused dialogue/menu state |

Core loading/audio/scene/checkpoint adapters are reused byte-for-byte. The text
compiler is extracted without the Lab's story graph; the game owns its story.
The painter's addresses are adapted to the actual layout: Plane A pages at
C000/A000, Plane B at E000, SAT B800 and horizontal scroll BC00. Two streamed
640-pattern slots occupy C0..33F and 340..5BF; resident font/actors/panels end
before C0. Byte-for-byte provenance checks protect the reused implementations.

## Findings and corrections

### Save syntax is not game semantics

The example codec originally constrained flags to the old Lab's values. Reusing
its CRC and commit protocol did not make its payload valid for this game. The
new application validates room, collision-safe position, packed Y and objective
invariants at both request and decode. Its distinct `MDGLASS__A_` / `MDGLASS__B_`
namespace prevents interpreting another Lab's records. Explicit saves keep copied
snapshots; scenes retain their lease until the complete acknowledgement retires.
Continue is revoked if the player opens another title page or starts another trip.

### Generated input is part of the build graph

The selected Megadev make rules track source files but do not infer project
headers or assembly `incbin` dependencies. A regenerated font looked correct on
disk while the assembled object remained stale. All standalone Lab Makefiles now
add explicit local-object dependencies and compile only the source prerequisite
(`$<`), avoiding the SDK recipe's all-prerequisites argument (`$^`).

The regression changes the actual font generator and movement header, rebuilds
without cleaning, executes the altered target movement and checks linked glyph
bytes, restores the inputs, then demands the original module payload back. This
checks rebuild behavior, not just a Makefile string. Tiny Labs conservatively
track all local headers/resources; a larger project can use precise dependency
manifests/depfiles. Clean/fresh builds remain necessary when changing flags or region.

### A valid glyph index can still draw nothing

Mixed-case game strings were paired with an uppercase-only font generator. Valid
lowercase indices rendered blank. The game now provides deliberate uppercase
aliases and its required punctuation, checks every letter's actual tile bytes,
and checks authored caption widths. Layout checks accompany—not replace—ares
visual review. Normal dialogue/control input waits for the shared painter's page
acknowledgement, avoiding commands applied to an unseen page.

### Bare modules need explicit runtime dependencies

Large aggregate clearing and apparently small unsigned modulo expressions can
emit `memset` or 32-bit libgcc calls. The selected Megadev modules link without an
automatic C runtime. The game uses bounded clearing/three-choice wrapping rather
than adding an implicit library dependency. Host O0/O3 tests alone cannot catch
these link/ABI issues; original-M68000 builds and execution do.

## Automated evidence

- Host game/codec checks at O0/O3 and UBSan outside Windows: progression,
  unprimed/wrong valve branches, cancellation, collision, pause, region clocks,
  copied saves and corruption recovery.
- Actual compiled M68000 game/renderer execution: full objective route, typed text
  gating, equal PAL/NTSC state, valid/invalid saved positions, complete UI-page
  flips, immutable copied Word RAM uploads, early-blank-only palette commits,
  bounded byte counts and invalid metadata rejection before MMIO.
- Shared paired-CPU streaming/audio suite: live drive/music handoffs, PCM sample
  residency/lifetimes, cancellation during read/validation/upload, missing/invalid
  files, CRC failure, ownership races, stalled grants and timeout drain/quarantine.
- Paired checkpoint/BIOS model: only this game's exact named records, no formatting
  or deletion, failed writes/unavailable media, late Continue rejection, save-vs-
  loading exclusion, lease retention and timeout/late-reply quarantine.
- Native CI ARM64/AMD64: US/JP/EU releases, US debug and automatic tour, with
  byte-identical target modules/audio required. Fail-closed reports require game,
  integration, checkpoint and incremental-build execution evidence.

The publisher uploads at most four UI rows, 160 map/upload words, two SAT entries
and changed palette lines: a conservative bound of **784 data bytes** per
submission. It copies Word RAM payloads before VBlank; no IRQ keeps a Word RAM
pointer. The first linked release used 0x39A0 bytes of Main BSS within a 0x4000
reservation; layout/link checks enforce the boundary rather than treating that
measurement as a permanent allowance.

## Manual and remaining qualification

Run the original game in ares in NTSC and PAL, including notes/dialogue, both
travel directions, audio handoffs, Save → Title → Continue and the ending. Test
returning to title during an entry preload. The automatic tour never writes BRAM.
Use a matching locally owned BIOS and specify Mega CD explicitly when launching.

Instruction execution, modeled BRAM/CD/MMIO and identical builds do not prove
physical console behavior. Real drive delays, BIOS variants, IRQ/upload headroom,
analog audio and physical peripherals still require hardware qualification.
Long streamed voice, arbitrary large-map streaming, localization and all 25 skills
at once are outside this fixture's claims.

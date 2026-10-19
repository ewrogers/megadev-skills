# Skill validation

## Run locally

Python 3.11+, a C17 host compiler and Docker are required. From the repository root:

```sh
python3 -m venv build/test-venv
build/test-venv/bin/python -m pip install -r tests/requirements.txt
build/test-venv/bin/python -m unittest discover -s tests -v
docker pull --platform linux/arm64 ewrogers/megadev:latest
docker pull --platform linux/amd64 ewrogers/megadev:latest
build/test-venv/bin/python tests/integration.py --platform linux/arm64 --output build/check-arm64
build/test-venv/bin/python tests/integration.py --platform linux/amd64 --output build/check-amd64
build/test-venv/bin/python tests/compare_reports.py build/check-arm64/report.json build/check-amd64/report.json
```

Output directories must be new. On Windows, use `build/test-venv/Scripts/python.exe`. Local Docker can emulate the other CPU; CI runs both architectures natively. CI disables install telemetry.

## Coverage

- Host C: button edges, bounds, one in-flight command, stale acknowledgments, success, fatal status, timeouts and stuck completion; O0/O3 and undefined-behavior sanitizer where available.
- Generator: installed-copy independence, spaces in paths, invalid metadata rejection and existing-file preservation.
- ISO parser: nested contents, empty filesystem and malformed extents/signatures/directories.
- Toolchain: four fresh builds per architecture: US/NTSC, JP/NTSC, EU/PAL and US debug. Expected disc files must equal compiled modules. A deliberately tiny RAM region must fail the link.
- Executed target code: resident, room and Sub startup copy initialized data, zero poisoned BSS, preserve guards and set expected stack pointers. An actual Sub interrupt callback increments before its tail-jumping access-loop continuation.
- Cross-architecture comparison: identical target file and bootstrap hashes, startup facts and case inventory; fails closed on missing/failed reports.
- Distribution: telemetry-disabled skills CLI install, exact packaged resources and generation without repository helpers.

These are not full dual-CPU, BIOS or physical-drive simulations. Regional builds do not prove every region's BIOS boots on hardware.

## Findings from this slice

The framework's ordinary make flag additions can be suppressed by command-line CC_FLAGS; the example exposes OPT_FLAGS instead. C startup and stacks need explicit work on both CPUs. A bootstrap BSS byte count cannot be used directly as a count of longword stores. PROCESS_ACC_LOOP tail-jumps, so the heartbeat belongs before it. Success from the CD loader is 0x64, not zero. Module imports bind to resident addresses, making an immutable disc important during tests.

The first emulator pass exposed a font palette mismatch and separate Mega CD input bindings. The corrected pass shows readable white text and advancing Main/Sub counters. The user also confirmed that module loading and return, Sub ping and marker movement work in ares.

No real-console test has been performed. Disc latency, BIOS/model differences and peripherals remain hardware checks.

## Planning skill

`megadev-planning` adds no target runtime code and does not need a new demonstration disc. Its standalone checker and worksheet complement Project Lab rather than replace its build/startup workflow.

Nine additional host tests cover exact fits and one-byte overflow in all six physical pools, overlapping lifetimes, 2M ownership, independent 1M-bank limits, H32/H40 scanline limits, separate CPU budgets at 50/60 Hz, assumption-based preload timing, malformed declarations and CLI exit codes. Both installed skills are checked for resource fidelity; the installed planning checker runs without repository helpers.

The existing native ARM64/AMD64 disc matrix remains enabled to catch accidental changes to the shared foundation. Planning calculations are host checks, not new hardware or emulator timing evidence. The example deliberately includes unmeasured values and warns about them.

See [planning validation cases](planning-validation.md) for decision-oriented walkthroughs and the remaining limits.

## Hardware Lab

`megadev-hardware` includes its own installed diagnostic disc rather than depending on files from another skill. Run the same integration helper with `--skill megadev-hardware` and new output directories for each architecture, then compare their reports. CI builds both Project Lab and Hardware Lab on native ARM64/AMD64.

Hardware Lab adds US release, JP release, EU release, US debug and US automatic-tour cases. Actual compiled probe routines execute on the original M68000 model on both Main and Sub, checking known endian patterns, corruption detection, zero-length refusal and guard bytes. Host tests run the primitives at O0/O3 with undefined-behavior checking where supported. Comparison rejects missing tours, missing target probe execution or mismatched skill reports.

In ares v148 with the user's configured US BIOS, the automatic tour completed three scratch/2M round-trip checks, a verified ping and two disc-module reloads, displaying all four buffer PASS results and AUTO TOUR PASS. This is emulator runtime evidence, not controller-input or physical-console qualification. Synthetic A-button events did not reliably reach the manual build even with background input allowed; the tour uses a deterministic input source to test operations independently.

The probes cover only reserved 256-byte local arrays and a 1024-byte Word RAM buffer. No 1M-mode, ASIC, audio, save-media or real-drive qualification is claimed. See the installed skill's verification reference for the remaining hardware checks.

## Asset Lab

`megadev-assets` adds strict native-resolution PNG preparation and an optional MDA1 teaching format. The checked-in PixelLab sources use exact requested palettes and dimensions: two 320×160 backgrounds and two 16×24 sprite cels. Palette JSON files keep LF endings across checkouts because reports hash their exact source bytes. Thirty generations were used; CI never calls the generation service. Prompts, originals and provenance ship with the Lab. Preparation preserves all visible pixels; transparent hidden RGB is normalized.

Nine asset host tests cover CRAM encoding, opt-in channel snapping, pattern nibble order, flip deduplication, sprite frame order, pixel round trips, malformed headers/maps/CRC, partial alpha, input preservation, stale files and path escapes. A host C fixture exercises the decoder at O0/O3 and with undefined-behavior checking where available. Distribution installs the fourth skill, generates its standalone Lab and runs the installed image converter and verifier.

Run `tests/integration.py --skill megadev-assets` for each architecture and compare the reports as above. Each five-case matrix (US/JP/EU release, US debug, US tour) regenerates all prepared images, compares exact bytes and executes the actual compiled decoder on the original M68000 model. Valid, corrupted and truncated inputs run with guarded output and stack checks. Disc payloads and bootstrap hashes must match across architectures. Local ARM64 Docker is native; local AMD64 is emulated. CI uses native runners for both.

The C debug build exposed a dependency on `__mulsi3` from bounded 32-bit geometry multiplication. Megadev's direct link does not automatically supply libgcc. This example uses a bounded integer helper; this is a link/runtime constraint, not evidence of a compiler defect or a rule against multiplication.

In ares v148 with the configured US BIOS, the backgrounds and retained animated sprite render with three palette lines; the automatic tour completed four loads and displayed AUTO PASS. Bulk loading is display-off; CRC verification is synchronous. This does not demonstrate bounded streaming or a measured frame-time guarantee. Both background planes need opaque panel tiles behind transparent font pixels; clearing only one exposed the scene's backdrop through the text. Keep BIOS VDP caches and hardware register changes consistent.

The Word RAM upload uses `BIOS_DMA_XFER_WORD_RAM`, while resident Main RAM uses `BIOS_DMA_XFER`. Emulator success does not qualify the documented physical-hardware final-word DMA workaround, drive latency, other BIOS/models or PAL display timing. Regional compilation and CPU tests are separate evidence from console tests.

## Graphics Lab

`megadev-graphics` separates prepared-art contracts from VDP rendering. Its standalone Lab loads a sky and quay through Word RAM, then keeps their patterns and two ship cels resident. Two scrolling planes, a fixed opaque Window HUD, hardware flips, sprite priority and four palette modes share one frame owner. PixelLab supplied four native-resolution outputs (30 generations); mirror extension creates repeatable layers without resampling or color substitution. The source sky's dithering is preserved, not imposed on other assets.

Run `tests/integration.py --skill megadev-graphics` for ARM64 and AMD64, then compare the reports. The five-case matrix includes US/JP/EU release, US debug and a US automatic tour. CI runs both architectures natively. Local ARM64 is native; local AMD64 uses Docker emulation. Reports retain the actual compiler/SDK revisions and target hashes.

Host tests exercise admitted/rejected sprite lists, extreme offscreen coordinates, SAT links/size/flip words, pattern extents, empty/full lists, independent H40 scanline entry and pixel limits, channel saturation, palette isolation/restoration, animation holds, pause and equal elapsed-time PAL/NTSC updates. C runs at O0/O3 with undefined-behavior checking where supported. Art tests reconstruct exact prepared bytes, check both repeating seams and reproduce the opaque font. Distribution installs all five skills with telemetry disabled and runs the generated Graphics Lab's verifier independently.

The actual compiled original-68000 model runs clipping, rejection and PAL/NTSC cases. The actual VBlank publisher executes with captured MMIO writes: tests compare every SAT, scroll, palette and HUD word in order, check both buffers, an empty replacement, an idle callback and rejection above the runtime cap. This is not a VDP/FIFO/IRQ timing model. Region builds, CPU execution and byte-count bounds do not prove real-console frame rates.

The Lab caps submitted sprites at 24 and transfers at most 484 data-port bytes per update. `UPLOAD` is a byte tally, not FPS, CPU usage or a promised general-purpose allowance. BIOS overhead and control-port writes are additional. Palette effects preserve the HUD, derive from an immutable base and update only through the frame owner; the BIOS's independent sprite/palette copy paths are not enabled alongside it.

In ares v148 with the configured US BIOS, the automatic tour completed with AUTO PASS, and the scrolling planes, animated/flipped ships, occlusion and steady Window HUD render correctly. The user also confirmed that testing the restored normal build looked good, covering the interactive movement, parallax, effects, priority and pause check. The automatic tour intentionally ends paused and labels that behavior on screen. Physical hardware, BIOS/model variation, actual Word RAM DMA behavior, PAL display timing, maximum sprite load, FIFO deadlines and CD/audio coexistence remain unqualified. The automatic tour separates runtime operations from unreliable synthetic controller input.


## Input Lab

`megadev-input` adds direct three/six-button pad, Mega Mouse, Team Player and
explicit two-port EA multitap readers. These are example implementations around
Megadev's hardware definitions, not newly discovered SDK driver APIs. The source
audit found a three-button macro, but no complete reader for the other devices.

Run the integration helper with `--skill megadev-input`. Six cases per architecture
cover US/JP/EU release, US debug, a scripted tour and explicit EA mode. Local
native ARM64 and emulated AMD64 builds pass and produce identical target payloads.
CI repeats them on native runners. Reports record the selected image and revisions.

Host tests cover all six-button masks, mouse signs/overflow, all supported mixed
Team Player type combinations, malformed/truncated packets, bounded acknowledgments,
idle-line restoration, EA selector/phase order, baseline suppression, failure release,
joining and equal-time PAL/NTSC repeat. Fixtures execute at O0/O3, with undefined
behavior checking where supported. Distribution installs all six skills with
telemetry disabled and reproduces the standalone Input Lab font.

Actual compiled 68000 code executes both port modes with scripted MMIO responses,
bus ownership/failure handling, IRQ-mask restoration and the bounded UI publisher.
The ISR's saved registers, Sub-interrupt call and exception frame are checked up
to its RTE instruction; the CPU harness cannot execute RTE. Ares exercises the
complete interrupt return. Mocked MMIO does not measure real peripheral timing.

In ares v148, the scripted tour reached AUTO PASS with six joined simulated
players and continued running. The user confirmed live D-pad/A/B/C/Start and a
steady display.
Port 2 was also recognized as a six-button pad and then a Mega Mouse; its original
Control Pad selection was restored. Physical X/Y/Z/Mode key checks were skipped
at the user's request. Mouse motion/button decoding has automated coverage, but
live mouse motion/button operation has not been qualified. Ares exposes no Team
Player or EA adapter option, so multitap runtime checks remain scripted.

Replacing BIOS joypad polling requires preserving the Sub interrupt service.
The Lab installs one input/frame owner and explicitly triggers Sub interrupt 2.
Its small IRQ-masked polling section preserves the previous SR and Z80 bus state;
it includes no audio driver. Real pads/mice/adapters, electrical timing, other BIOS
models, PAL display operation and concurrent Z80 audio remain hardware/integration
checks. The on-screen scripted mode is labeled and never presented as live input.


## System Lab

`megadev-system` owns command lifetimes and service coordination. It links to
project startup, hardware facts, input and rendering guidance instead of defining
another allocator, asset pipeline or device driver. Its standalone disc uses a
single-slot project protocol; the `sy_*` helpers are not Megadev APIs.

Run the integration helper with `--skill megadev-system`. Five cases cover
US/JP/EU release, US debug and an automatic US tour. Native ARM64 and local
emulated AMD64 execute both compiled original-68000 adapters against a shared
mailbox/Word RAM model. They check publication order, writer direction, every
owned buffer access, normal/rejected/cancelled/late results, varied observation
order, skipped/repeated worker ticks, cancellation just after completion and
refusal to reuse a buffer after a grant failure. CI runs both architectures
natively and comparison requires executed checks on both CPUs.

Host fixtures cover input/output/checksum guards, bounded chunks, invalid
counts/opcodes/headers, corruption, owner refusal, cancellation at each chunk,
stale replies, retirement and PAL/NTSC deadlines. A response first observed
after a foreground stall expires is discarded. C runs at O0/O3 with undefined
behavior checking where supported. Distribution verifies all seven installed
skills and independent System Lab creation/font regeneration, with telemetry off.

The paired CPU harness needed to retain each Unicorn Work RAM backing allocation
with its CPU instance; previously single-CPU checks completed before that local
allocation expired. The harness models ownership grants, not electrical timing.
The IRQ register-save, clock and input-edge accumulation execute up to RTE, with
BIOS calls stubbed. Full interrupt return remains an emulator/hardware check.

The first ares tour exposed a clock error: UI callback counts advanced more
slowly than physical VBlanks. The corrected standalone handler counts every
VBlank and retains input edges between UI updates. Main deadlines use elapsed
interrupt ticks, independent of redraw frequency. Sub INT2 still runs the CD
access-loop continuation, and ordinary worker chunks run in Sub foreground.

In ares v148 with the configured US BIOS, the corrected tour reached AUTO PASS
with 2 accepted, 1 rejected, 2 discarded and 0 bad-data jobs; Main/Sub clocks and
the control light continued. The user also confirmed manual movement, job
processing, delayed-job recovery, cancellation, rejected requests and steady text.
No physical-console timing or audio/drive
coexistence qualification is claimed. Bootstrap disc loading is synchronous;
the runtime example transforms a 522-byte owned buffer, not asynchronous CD data.


## Memory Lab

`megadev-memory` adds concrete linked RAM accounting, transient allocation
lifetimes and 1M/2M Word RAM view validity. Planning still owns estimates;
system owns request/acknowledgment contracts; graphics owns VDP transfer service.
The arena, leases and memory commands are project examples, not Megadev APIs.

The standalone layout reporter checks Main's 64 KiB and Sub's 512 KiB physical
capacities, explicit reservations, actual module extents, overlaps and embedded
pool bounds. The 4 KiB scene arena must exist in the linked state and is reported
without adding it twice. A diagnostic-only write initially disappeared under
optimization, so the Lab backing now uses observable volatile accesses; this
does not prescribe volatile for general game assets. Reserved stacks/windows
and measured runtime usage remain distinct.

Host fixtures cover exact fits through 128 KiB, alignment, overflow/full refusal
without mutation, pool/generation checks, leased unload refusal, stale handles,
generation exhaustion, guarded chunk filling and corruption. Layout tests reject
wrong capacities, overlap, one-byte overflow and a payload larger than a 1M bank.

Run the integration helper with `--skill megadev-memory`. Five cases per
architecture cover US/JP/EU release, US debug and US tour. Local native ARM64 and
emulated AMD64 pass and produce matching payloads/layout reports. CI uses native
runners for both. Compiled original-68000 adapters execute against one physical
256 KiB model with interleaved 1M words and complementary CPU assignments.
Every buffer access checks its view/ownership. The test rejects Main's 220000
cell-mapped alias as an independent second bank and Sub's wrong-mode views.

Paired tests exercise both RET directions, Main-front reads during Sub-back
writes, different prior ownership on 1M exit, skipped/repeated ticks, pinned-reader
swap refusal, early release, stale handles and a grant failure that cannot
authorize reuse. The model covers register visibility, not electrical timing,
arbitrary instruction interleavings or a full CDC/ASIC/BIOS machine.

In ares v148 with the configured US BIOS, the automatic tour completed nine
memory operations plus the lifetime check and reached AUTO PASS with zero
errors. Both clocks and the diagnostic display continued. This includes 2M
handoff, 1M entry, repeated swaps/fills and return to 2M with rebuilt contents.
The user also completed the manual Budgets, Lifetimes and Word RAM checks on
2026-10-17: allocation refusals, release/unload and the prepare/1M/swap/2M
sequence worked, with Errors remaining `0000`.
Physical mode/ownership timing, other BIOS/models, real stack maxima, PAL display
operation and DMA/disc/audio/ASIC coexistence remain unqualified. No runtime
disc streaming, direct Word RAM VDP DMA or ASIC work is demonstrated.

All eight skills install with telemetry disabled in distribution checks.
Memory Lab's independent generator, font regeneration and refusal to report an
unbuilt layout are exercised. README, skill groups and roadmap include memory.

## Streaming Lab

`megadev-streaming` adds a CD-native area preload example. It uses the pinned
Megadev `filename`, `filebuff`, `access_op`, `access_op_result` and `filesize`
contract, with request parameters committed before the operation. The resident
Sub INT2 access service stays alive while Main renders resident art. A C ABI
shim executes upstream `find_file` to validate directory size before starting a
sector-rounded transfer. It does not use the pinned C inline wrapper's RTS path.

Two original Asset Lab pictures remain unchanged, with a resident actor/font.
Prepared files are checked before building; generated size constants must match
the directory. A reserved 32 KiB portion of 2M Word RAM holds the padded read.
The Lab explicitly does not qualify a 1M CDC/DMA pipeline. Main reads only after
the matching result, confirmed ownership and mailbox retirement. CRC/map checks
advance by at most 128 bytes per tick; pattern uploads are 64/256 bytes per frame.
Two 640-tile slots end at B800 before the BIOS SAT. Main release code/data image
is 11,936 bytes and linked state is 7,816 bytes in the audited build; reserved
windows/stack capacity are distinct from those measured extents.

Portable C tests exercise sector rounding, overflow, exact capacity, malformed
headers/palettes/maps and corrupt checksums at O0/O3 with undefined-behavior
checking where supported. The full host suite has 47 tests. Paired compiled
original-68000 tests execute both adapters, real directory lookup and its ABI
shim against a synthetic delayed sector producer. They cover overlapping
requests, cancellation during read/validation/upload, missing/oversized files,
read failures, corrupt data, timeout draining, unconfirmed grants and reuse only
after retirement. Uploads remain unpublished until complete; VBlank consumes
copied Main RAM bytes rather than retaining a Word RAM pointer.

The publisher originally rewrote CRAM every frame after HUD writes. It now sends
only changed palette lines, first, with an early-VBlank status/counter guard;
late updates retry without changing the displayed palette snapshot. HUD work is
limited to 24 changed cells. Tests verify no CRAM writes during active display
or late blank, one early update and no repeat for unchanged colors. Map changes
wait until the displayed area palette is black, rather than trusting a queued
fade. In ares v148 with the configured US BIOS the revised tour reached AUTO
PASS after preload, warp, cancelled read and missing-file recovery, with Errors
0000 and foreground movement continuing. No CRAM dots were apparent in the
revised capture; manual visual checks remain separate evidence.

US/JP/EU release, US debug and US tour are built per architecture. CI uses native
ARM64/AMD64 runners and compares target payloads; local AMD64 is emulated on the
ARM64 host. Docker build/test containers use a two-CPU quota. Installed-resource
checks include the independent generator, font and prepared-art tools with
telemetry disabled. README, skill groups and roadmap include streaming.

The synthetic producer is not a CDC/BIOS/drive timing model. Ares exercises the
real reader/BIOS/VDP path but does not establish physical seek/retry deadlines,
stack high-water, console/model compatibility or CD-DA/PCM/ASIC/DMA coexistence.
No compressed decoding, general tile-column cache or seamless level streamer is
claimed by this small area-preload example.

The user confirmed the manual US build: controls, fades and recovery worked with
no CRAM dots. They confirmed the slowdown occurred only at the far-right exit, not during
ordinary movement. The original
preload trigger allowed only 59 movement ticks before the boundary, versus at
least 166 validation plus 77 upload ticks for the cavern. The Lab now starts
preloading during entry and explicitly labels an exit that is still waiting.
This removes the unexplained boundary behavior; it does not claim that real CD
loads or the deliberately slower upload mode always finish before arrival.

## Audio Lab

`megadev-audio` covers CD-DA tracks, resident PCM effects and music/data-drive
handoffs. The standalone Lab authors two original stereo CD tracks and three
effects plus a looping bed. Its mixed-mode CUE includes real silent pregaps;
opening only its ISO cannot test the music. No SGDK driver calls or unverified
FM/PSG music driver are presented as Megadev APIs.

Local native ARM64 and emulated AMD64 passed US/JP/EU release, US debug and US
automatic-tour builds. All target payloads, bootstrap hashes and external audio
track hashes match. The selected `ewrogers/megadev:latest` uses GCC 16.2.0 and
Megadev revision `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Native CI repeats
both matrices. Containers remain limited to two CPUs.

Portable C fixtures check voice priorities, bounded lifetimes, pause/resume,
drive exclusion, latest music intent and faults at O0/O3, with undefined-behavior
checking where supported. Authoring tests cover sign/magnitude samples, reserved
loop markers, bank extents, release tails, pitch steps, WAV sectors, pregaps and
CUE preservation. The complete host suite has 52 tests. Distribution checks
install all ten skills with telemetry disabled and generate the audio independently.

Compiled original-68000 adapters execute PCM bank/byte-lane uploads, channel
enable masks, pitch/pan, mailbox retirement and a BIOS completion stub. Lost
acknowledgments and stuck commands quarantine their slots rather than granting
unsafe reuse. Paired reader tests retain delayed/failed reads, Word RAM ownership,
corruption checks and copied VBlank uploads. These models neither synthesize
sound nor measure BIOS or physical-drive timing.

In ares v148 with the configured US BIOS, the automatic US tour reached
`AUTO PASS - AUDIO CONTINUES` after its music/PCM requests and area handoff.
The cavern rendered and the resident PCM bed remained marked active. This is
runtime/command evidence, not an audible-quality result.
The user confirmed that both CD music and PCM sounds work in the manual US
build in ares on 2026-10-18. Detailed pitch/mix quality, track repeat, real seek
behavior and PAL listening remain separate checks. Physical consoles,
BIOS/model variations, FM/Z80 drivers, PCM streaming and ASIC coexistence remain
unqualified. The Lab intentionally stops CD music for data reads and restarts it
afterward; resident PCM can continue during the handoff.

## Transition Lab

`megadev-scenes` composes the existing reader, audio owner and VBlank publisher.
It adds resident game-flow states rather than treating every scene as a module.
Its title, options, credits, two areas, pause and results are illustrative. Shared
art/audio and two bounded area slots stay resident; local collected-lantern state
is cleared only after a successful scene commit. Retained cache bytes are not
misreported as freed scene memory.

Portable C fixtures execute at O0/O3 with undefined-behavior checking where
supported. They cover black-display acknowledgment, leases, stalled retirement,
missing-target rollback preserving progress, latest queued requests, stale
generations, generation exhaustion and equal-time PAL/NTSC fades. The complete
host suite has 56 tests. All eleven skills install with telemetry disabled, and
the standalone Transition Lab generator/art tools run without repository helpers.

Compiled original-68000 checks execute both the portable scene policy and the
actual Main coordinator around an outstanding reader, held generation, unretired
mailbox and acknowledged map replacement. Existing audio and paired-reader tests
also execute against the Lab. Completion signals are modeled, not an arbitrary
instruction-interleaving or full drive/VDP timing simulation.

Local native ARM64 and emulated AMD64 pass US/JP/EU release, US debug and US tour;
their payloads, startup facts and external CD-track hashes match. Native CI repeats
the matrices. The selected latest image reports GCC 16.2.0 and Megadev revision
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Build/test containers use two CPUs.
The first build exposed implicit 32-bit division helpers in the fade clock;
Megadev's direct module link does not automatically add libgcc. A loop bounded
to eight useful fade steps avoids that dependency without changing fade timing.

In ares v148 with the configured US BIOS, the tour reached
`AUTO PASS - CLEANUP AND RECOVERY`: nine scene changes, cancelled-preload drain,
re-entry and missing-area rollback completed. The normal US disc is open for
manual movement, pause, UI and listening checks. Physical seeks/retries, other
BIOS/models, stack maxima, PAL presentation and audio quality remain separate
checks. Dynamic module unloading, 1M streaming and ASIC coexistence are not
demonstrated.

The first manual pass found that the beacon used an undefined opaque HUD glyph.
It now uses a dedicated transparent 16x16 white/gold marker, with native pattern
and compiled sprite-chain regression checks.
The user confirmed the corrected beacon is visible and reaching it then pressing
A continues normally in ares on 2026-10-18. The full manual pause/preload/audio
matrix remains separate from that confirmed visual/input check.

## Save Lab

`megadev-save` adds actual internal `_BURAM` BIOS files, explicit versioned
records and alternate-record recovery. It keeps its Sub workspace resident and
uses a single Main/Sub request slot. Normal save/load/recovery only touches its
private filenames and never formats. A separate confirmed Format Device action
erases all saves on the selected device through BIOS command 6. A separate Main-side `BRAM_CART` backend uses the same checked records and an
explicit device selector. Capacity, archive/copy workflow and IO boundaries are
documented from the official manual and primary sources. Copying between devices
and custom Work RAM handlers are not implemented.

The 59-test host suite includes O0/O3 C fixtures and undefined-behavior checks
where supported. Save cases cover a known CRC vector, every single-bit record
error, guards/output preservation, v1 migration, future-version refusal, sequence
exhaustion/conflicts, failed/partial writes, readback mismatch and older-record
recovery. Twelve skills install with telemetry disabled and generate independently.

Original-68000 execution checks the actual Main/Sub service and assembly BIOS
ABI: resident BRMINIT buffers, carry handling, A0 reset after search, exact private
names, one normal 64-byte block and BRMWRITE's D1.l zero. Oversized/protected
files stop before a BIOS read. Unformatted/unavailable media stops at init for ordinary requests.
The BIOS model rejects deletion and any format outside the explicit test flow. Request snapshots, retirement,
50/60 Hz timeout quarantine and the actual recovery-menu dispatcher execute.
These model BIOS outcomes; they do not establish physical battery/write timing.

US/JP/EU release, US debug and a read-only US tour pass locally on native ARM64
and emulated AMD64. Native CI repeats both matrices and compares target bytes
and execution facts. The latest container reports GCC 16.2.0 and Megadev revision
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`; containers use a two-CPU quota.
The automatic tour only probes media and deliberately never creates or corrupts
emulator save files.

In ares v148 with the configured US BIOS, the user confirmed save/load and steady
text. A normal emulator quit/reopen retained sequence 0006 and the new BIOS probe
validated it. The original B+A recovery gesture was awkward because B could
enqueue a load before A; Help now exposes a direct A recovery action, with B only
returning to values. Compiled menu tests prevent that race from returning.
The user confirmed that the revised action recovered the previous save's values.
Physical consoles, depleted batteries,
shared filesystem damage, full/fragmented media, other BIOS variants, PAL runtime
presentation and simultaneous CD/audio/ASIC activity remain separate checks.

The cartridge addition executes Main ABI shims with an independent named-file
model, valid standard IDs, absent/custom IDs without handler dispatch, unformatted
media and selection without internal fallback. An optional local fixture also
executes the user-supplied US BIOS and compiled Main adapters on original-68000
with fresh 16/128/512 KiB cart models: save, readback and corrupted-newest recovery
pass. That fixture formats only its fresh Python bytearray, never emulator save
files or hardware. No firmware/disassembly is committed. Its exact firmware hash
and reproduction command are documented. Ares v148 lacks the CD RAM-cart mapping,
so its manual evidence remains internal-only; actual cart hardware or a cart-capable
emulator still needs interactive qualification.

Explicit formatting now covers both internal RAM and standard backup carts.
The confirmation names the selected device, warns that every game's saves there
will be erased, and opens on Cancel every time. B/Start cancel, and navigation
combined with A cannot also confirm. Compiled tests exercise both adapters,
independent stores (including unrelated filenames), absent-media refusal,
format failure, failed re-probe, zeroed sequence and subsequent save/load. The
optional local BIOS fixture executes the compiled cart formatter rather than
seeding its filesystem with a direct BIOS call. Formatting is tested on fresh
in-memory storage; existing user emulator saves are left intact.

## Scaling Lab

`megadev-scaling` adds an actual Sub graphics-ASIC path, not a software rasterizer
presented as hardware acceleration. The original 256px observatory mosaic uses
one palette line and native 16px stamps. Zoom, orbit and per-scanline perspective
floor all render to a fixed 128×128 Word RAM image. The US release links 17,028 bytes of Main BSS and 1,064 bytes of Sub BSS,
including the snapshot, HUD queue and trace scratch; these are linked example
costs, not universal engine reservations. Main copies a completed
snapshot and stages it into one of two hidden 8 KiB VRAM banks before committing
the name table. A separate wisp remains independent of the transformed image.

The host suite now has 63 tests. Scaling C fixtures cover 3 modes × 256 heading
phases × 5 scale settings, fixed-point limits, identity/quarter turns, monotonic
floor sampling, malformed requests and output guards, at O0/O3 with undefined-
behavior checks where supported. Original native art/palette/stamp bytes rebuild
exactly; the installed generator/art tools run without repository helpers and
all thirteen skills install with telemetry disabled.

Compiled original-68000 Main/Sub tests check trace agreement, register units and
write-to-start order, masking only ASIC INT1, delayed completion, actual ownership
before access, independent image snapshots, and timeout quarantine for both a
stuck grant and stuck ASIC. Functional ASIC sampling is a model for the fixed
profile, not a hardware timing simulator. The actual VBlank callback executes
against captured port writes/a DMA stub: active/late blank deferral, resident
source bounds, chunk contents and complete-image publication are checked. The
next ASIC operation may overlap DMA, but cannot overwrite a leased Main snapshot.
Changed-cell HUD tests cover coalescing, unchanged redraw and shorter labels.

US/JP/EU release, US debug and the automatic tour are built separately on ARM64
and AMD64. CI uses native runners; local AMD64 is Docker emulation. Containers
have a two-CPU quota and reports retain actual compiler/SDK versions and hashes.
The selected latest image uses GCC 16.2.0, original 68000, and Megadev revision
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`.

The first link exposed wide arithmetic helpers not supplied by the ordinary
Megadev module link. The Lab uses a reciprocal table and a bounded signed
16×16→32 `muls.w` primitive, checked through actual target execution. It does not
assume later m68k instructions or teach that every C multiplication needs assembly.

In ares v148 with the configured US BIOS, the initial automatic tour reached
AUTO PASS after images from all three views with zero errors. The user questioned
the image cadence; the new rate counter exposed approximately five images per
second in the first serial/word-copy/full-HUD-scan version. A fixed 8 KiB MOVEM
snapshot copy, overlapping render/upload, changed-cell HUD queue and prompt
ownership/retirement rechecks removed the bottlenecks. The corrected manual
build showed 30 new images per 60 serviced VBlanks, including the perspective
floor, with zero errors. This reaches the example's two-upload NTSC ceiling;
it is not the ASIC's universal limit, a game-loop FPS claim or physical hardware
qualification. Temporary phase profiling was removed; IMAGE/S remains decimal
and the cumulative IMAGES count remains hexadecimal.

The faster schedule also exposed an ownership-snapshot race. Each CPU now reads
ownership after observing the other CPU's committed token/reply, rather than
rejecting a valid request using an earlier mode sample. Adversarial compiled
tests deliver those events between the polling instructions. Ares evidence is
separate from that deterministic regression.

Real ASIC/bus timing, VDP/FIFO deadlines, stack high-water, other BIOS/model
variants, physical PAL/NTSC behavior and combined CD/audio/PCM/DMA workloads
remain separate qualification tasks. No BIOS or emulator source is distributed.

The user confirmed on 2026-10-18 that the corrected manual build's three views
and controls work and ERRORS stays 0000. This confirms the interactive emulator
check; it does not replace the physical-hardware qualification above.


## Debugging milestone

`megadev-debug` reuses the portable coordination model unchanged and instruments
its Main/Sub adapters with private 212-byte rings. Host tests cover wrap/order,
freeze/clear, overwrite saturation and invalid lookups. Paired original-68000
execution adds guard corruption, stable first-fault snapshots across later jobs
and timeout/drain evidence to the existing stale-reply, cancellation, ownership
and callback checks. Real ELF lookups run inside the container for all three
modules, including source attribution in the explicit `-g` build.

The five-case US/JP NTSC, EU PAL, debug and tour matrices run on native ARM64 and
AMD64 CI; payloads and executed markers must match. Distribution covers fourteen
skills with telemetry disabled. This is controlled protocol/data fault injection,
not verified fatal exception capture. ares checks readability and interactive
recovery; physical-console timing, BIOS variants and fatal handlers remain unqualified.


## Testing milestone

`megadev-testing` adds a 5824-byte in-memory replay fixture and a standalone JSON
verifier. Host O0/O3 checks cover reset, full/invalid tapes, input/event bounds,
first divergence, state-field coverage and 240 NTSC versus 200 PAL VBlanks for the
same four-second simulation. Compiled original-68000 checks execute the same
logic and suite coordinator. Its service suite reuses the actual streaming/audio
adapters; a combined execution model rejects music commands during a live reader
and checks stop/read/resume, cancellation, bad content, missing files and recovery.

A new race regression fails the old compiled streaming adapter: reading ownership
before a newly committed reply can use stale mode and falsely fault. Streaming,
audio, scenes and testing now read the reply before fresh ownership. Their existing
execution matrices all run this regression; no wire protocol changed.

The testing matrix covers US/JP NTSC, EU PAL, explicit debug and automatic-tour
builds on native ARM64/AMD64 CI. Complete reports require replay and combined
integration evidence, audio-track hashes and matching target payloads. Distribution
checks include all fifteen skills with telemetry disabled. The course uses scripted
service outcomes; real CD timing is tested separately, not claimed deterministic.
Ares/physical-console evidence remains distinct from host and CPU models.


## Performance milestone

`megadev-performance` adds reference/incremental work equivalence (1,200 host
phase/load combinations), independent original-68000 output checks, twelve-bit
stopwatch wrap/coherence rejection, timer-off/no-reset checks, bounded windows,
HUD-off and drain/warm-up policy tests. The real compiled Main/Sub ASIC pipeline
keeps the existing image/ownership/early-blank publication regressions.

US/JP NTSC, EU PAL, debug and tour matrices run on native ARM64/AMD64 CI with
matching payloads/results. Distribution includes sixteen skills, telemetry off.
Model-injected timer values are not hardware timing. The Lab reports serviced
callbacks, elapsed samples and completed work, never fabricated CPU utilization;
shared clock ownership, interrupt masking, real drive/PCM contention and physical
console/BIOS timing remain explicit qualification limits.

## Release milestone

`megadev-release` uses the unchanged Audio Lab rather than another target runtime.
The standard-library helper checks the selected SDK boot/security layout, bounded
ISO level-1 contents, exact compiled files and multi-file PCM WAV CUE semantics.
Its explicit-file ZIP is verified against a pinned contract after packaging.

Host regressions inject malformed extents/endian fields, security damage, broken
tracks/indices, unsafe paths and incomplete/tampered packages. Native ARM64/AMD64
audio jobs exercise US/JP NTSC, EU PAL, debug and tour archives with real regional
blocks and restored-input failure tests. Same-input ZIP determinism is checked;
whole-ISO reproducibility is not claimed because build timestamps may differ.
Existing original-68000 reader/audio/callback tests remain in the matrix.

Installation covers seventeen skills with telemetry disabled. Archive structure
and CPU models do not certify BIOS compatibility, optical media or real drive/PCM
timing. Interactive emulator and physical-console results remain separate evidence.

## Best-practices integration milestone

Harbor Relay adds checkpoint glue to the existing scene, streaming and audio
implementation. Owner-policy/adapter copies, save codec and release helper have
byte-identity regression checks; only the BRAM filenames differ from the save ABI
wrapper. No competing interrupt hook, renderer, reader or audio manager is added.

Host O0/O3 checks cover byte alignment, bounded arithmetic and equal nominal
PAL/NTSC elapsed time. Compiled original-68000 tests join both register views and
a named-file BIOS model: copied inputs, busy/retirement rejection, drive exclusion,
failed/unavailable saves, older-record recovery, scene leases, stale menu intent,
map-commit restoration and timeout quarantine. Format/delete are rejected.

Native ARM64/AMD64 matrices build US/JP NTSC, EU PAL, debug and non-writing tour
variants. Existing scene/CD/audio regressions remain active, and mixed-mode
release packages are verified. Installation checks include all eighteen skills
with telemetry disabled. CPU models do not certify real BIOS save latency, drive
seeks, interrupt timing, stack high-water or analog audio continuity; these remain
emulator/physical-console qualification tasks.

## UI milestone

Menu Lab checks native bitmap fonts, nine-slice bounds, fixed counters, bounded
navigation/history and focus restoration without adding a competing scene or
input manager. Host O0/O3 checks cover malformed state, shorter fields, held
buttons, equal nominal PAL/NTSC repeat time and publication lifetimes.

Original-MC68000 execution captures the real row publisher: deferred late/active
callbacks, immutable in-flight storage, physical row stride, final-only page
switches, BIOS register-cache consistency and fail-closed invalid batches.
ARM64/AMD64 build US/JP NTSC, EU PAL, debug and tour variants with matching payloads.
Installation covers nineteen skills with telemetry disabled. Native-resolution
PNGs come from the actual C tilemaps and prepared pattern/palette bytes.

The user confirmed NTSC and PAL navigation and steady display in ares, including Pack,
Details/Character, focus restoration, Settings and pause. Host/CPU tests do not
emulate the VDP or certify transfer timing, safe video margins, input latency or
physical-console/BIOS compatibility. Those remain separate checks.

## Dialogue milestone

Dialogue Lab adds bounded spans, wrapping/paging, styled reveals, choices and
paired portrait panels over the established UI primitives/publisher. Host O0/O3
checks cover overflow without partial output, input edges, ready generations,
PAL/NTSC reveal timing, both branches and stale voice intent. Source art/PCM
conversion is reproducible, and C-rendered native previews cover thirteen states.

Original-MC68000 tests execute script/view policy and the compiled tour. Paired
Main/Sub audio tests retain existing driver/BIOS coverage while checking the new
resident speech slots, finite lifetime, speech-only stop, independent effects,
invalid requests and cancellation before an old start's acknowledgement. Lost
acknowledgements retain the request rather than freeing it.

US/JP NTSC, EU PAL, debug and tour builds run on ARM64/AMD64 with payload matching.
Installation covers twenty skills with telemetry disabled. The user confirmed
NTSC and final PAL dialogue, branches, paired panels, voice and controls in ares.
The Lab preloads
short PCM excerpts; it does not prove runtime voice CD reads, optical timing,
real PCM acoustics or physical-console behavior.

## EverDrive disc iteration and capture

`megadev-project` adds native host Python/pySerial disc deployment and build-then-
deploy, with hard reset as the default. `megadev-testing` adds bounded direct
FFmpeg/AVFoundation capture. `megadev-hardware` extends its existing Lab with
native graphics, source/configuration build ID, resident heartbeat, EDIO
detection and a bounded cooperative host handshake. Each skill carries its own
runtime resources; no SGDK or repository-test dependency is installed.

Host tests cover complete multi-file transcripts under fragmented I/O, legacy
and extended Pro discovery, explicit soft override, reset-release cleanup,
transfer/install failures without launch, failed build without USB, unsafe CUE
paths/aliases/symlinks, immutable ISO/CUE snapshots, PCM track validation and a
copied CLI using real pySerial over a pseudo-terminal. Capture tests cover
explicit devices, no audio, no overwrite, bounded sampling and incomplete output.

The Hardware Lab target matrix includes US/JP NTSC, EU PAL, US debug, automatic
tour and guest polling disabled. Original-MC68000 execution checks Main/Sub
scratch primitives and the guest signature/CRC/nonce/parser/lease contracts,
including absent hardware, corruption, stale ACK and timeout negatives. CPU
execution does not emulate USB, the MCU, BIOS, VDP or optical I/O. Local ARM64
and emulated AMD64 builds are compared; CI runs both architectures natively.

Selected image was pulled before validation: Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0, binutils
2.47.20260726 and genisoimage 1.1.11. All build containers use `--cpus=2`.

Physical acceptance on 2026-10-19: the initial user-reported MEGAOS 4.20 setup
accepted ISO+CUE deployment and passed the Lab, but lost host USB during CD/BIOS
startup. After the user's firmware update, USB stayed available and repeated
live handshakes passed. `build-deploy` hard-reset and replaced the running disc.
The user pressed Start at the stock US BIOS 2.00 prompt. Direct HD60 X frames
showed replacement build `01768DE5`, Main/Sub startup PASS, all four buffer
checks PASS, three RAM checks, two reloads, AUTO TOUR PASS, EDIO DETECTED, USB
VERIFIED and changing counters/heartbeat. Detailed identities, hashes and
primary sources are in [the research record](research/mega-everdrive-pro-sega-cd.md).

This is EverDrive FPGA CD-core acceptance, with base console model unspecified.
It does not qualify a physical Sega CD drive, all regional BIOSes, PCM/CD-DA
coexistence, 1M mode, ASIC, saves, input mappings or fully unattended BIOS boot.
Generated discs, captures and logs remain ignored under `build/`.

The final UI caption follow-up compiled and ran as build `FAAC2829`, retaining
AUTO TOUR PASS and showing PASS/LIVE during fresh exchanges, then PASS/IDLE after
the host stopped. The completed five-second lease no longer looks like a failed
readiness result; PASS/IDLE explicitly records the earlier successful check.

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

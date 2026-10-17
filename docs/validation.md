# Skill validation

## Run locally

Python 3.10+, a C17 host compiler and Docker are required. From the repository root:

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

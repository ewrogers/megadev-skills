# QA automation and artifacts

The standalone `tools/verify_replay.py` compiles the portable C fixture with a host
C17 compiler, runs assertions and prints a compact JSON result. Exit 0 means the
checks passed; exit 1 is the intentional bug demonstration; other nonzero exits
are harness/build/assertion failures. Pass `--cc` to select a host compiler.
It uses a temporary output directory and does not require Docker or an emulator.

```sh
python3 tools/verify_replay.py
python3 tools/verify_replay.py --inject-bug
```

The second command must exit nonzero and report first divergence at tick 0.
Normal qualification also changes a recorded service outcome and detects its
first divergence at tick 180. Expected final state hash is a reviewed fixture
expectation, not an automatically regenerated golden result.

For a game's CI, keep the smallest representative scenarios and semantic assertions.
Report case ID, build/disc/module hashes, SDK/compiler identity, region/video target,
input/event tape version, initial state/seed, logical progress and first mismatch.
Retain logs and the failing reproduction even when later cleanup fails. Put a
bounded wall-clock timeout around the harness; do not turn a hung test into PASS.

Repository CI installs every skill in a disposable project with DO_NOT_TRACK=1
and DISABLE_TELEMETRY=1, verifies installed resources and runs the installed helper.
It builds native ARM64/AMD64 matrices, executes the selected CPU checks and rejects
incomplete or mismatched reports. Test dependencies stay outside release discs.

Host replay JSON is useful for unattended QA; it does not inspect an ares frame or
listen to audio. If adding emulator automation, qualify the specific capture/input
interface, distinguish a capture failure from a ROM failure, and preserve the
user's configuration, BIOS and saves. Prefer direct semantic probes over fragile
OCR when both are available. Keep a separate manual visual/audio checklist.

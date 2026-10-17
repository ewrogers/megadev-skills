# Hardware Lab

An interactive Megadev diagnostic disc for safe scratch-buffer checks, a 2M Word RAM round trip, CPU ping and module reloads. It derives its standalone boot/build foundation from this repository's Project Lab, with Megadev bootstrap portions under `LICENSE.megadev`.

Generate a new copy from the installed hardware skill directory:

```sh
python3 scripts/create_project.py /path/to/hardware-lab
```

From that new directory, build with the current Docker image:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue hardware.iso
```

PowerShell uses `source=$($PWD.Path)` in the mount argument. Optional Linux `--user "$(id -u):$(id -g)"` avoids root-owned output. Use Linux containers on Windows/macOS. The copied template itself has Makefile metadata placeholders; generate a project before building it.

Load the CUE as Mega CD. On macOS:

```sh
open -a /Applications/ares.app --args --system "Mega CD" /path/to/hardware-lab/hardware.cue
```

Use a matching BIOS already supplied by the user; none is included or downloaded. Start boots from the BIOS. Mega CD input mappings can differ from Mega Drive mappings.

## Controls and results

- A: run all four reserved-buffer checks. Each should show PASS.
- B: ping Sub. The reply should change and both counters should keep advancing.
- C: load the disc module. Its startup checks should pass. A/Start returns; try loading it twice.

Counters and raw mode/status values are hexadecimal. The raw mode word contains unrelated flags as well as ownership. It is not a byte count. The initial reply may be 0064, the bootstrap's successful CD result.

Only 256-byte scratch arrays on each CPU and a 1024-byte Word RAM buffer are tested. PASS does not mean the entire console RAM or all hardware passed. No saves are changed.

## Automatic runtime tour

Clean before changing build flags, then build with `TOUR=1` and recreate the CUE. The tour runs three buffer checks, one ping and two module reloads, retaining any failure before displaying AUTO TOUR PASS. It does not consume gamepad input and therefore is not an input test.

To restore manual controls, clean and rebuild with `TOUR=0` (the default). Build to a separate directory and reload a fresh disc; do not overwrite an image ares is still using.

For EU/PAL use `REGION=EU VIDEO=PAL`; Japan uses `REGION=JP VIDEO=NTSC`, with a matching BIOS and a clean rebuild. Default is US/NTSC. CPU-only CI covers all three region builds, US debug and US automatic tour. Regional builds are not regional real-console boot tests.

The Lab intentionally leaves 1M-bank switching, ASIC rendering, PCM/CD-DA audio, save media and drive-error recovery to focused system tests. Its existing BIOS and ownership polling helpers are not all bounded by the runtime command timeout.

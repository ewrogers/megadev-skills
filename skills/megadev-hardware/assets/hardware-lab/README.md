# Hardware Lab

An interactive Megadev diagnostic disc for safe scratch-buffer checks, a 2M Word RAM round trip, CPU ping and module reloads. It derives its standalone boot/build foundation from this repository's Project Lab, with Megadev bootstrap portions under `LICENSE.megadev`.

Generate a new copy from the installed hardware skill directory:

```sh
python3 scripts/create_project.py /path/to/hardware-lab
```

From that new directory, build with the current Docker image:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue hardware.iso
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

Counters are hexadecimal. Main/Sub C startup checks cover initialized data and zeroed BSS. The top-right eight-digit build ID hashes source, art generator and build configuration; it is not the ISO hash. The footer heartbeat and both counters must change across observations. Native fonts, a pixel disc graphic and a colored panel use resident Main RAM assets and VDP tiles; no SGDK runtime or external art tool is required.

Only 256-byte scratch arrays on each CPU and a 1024-byte Word RAM buffer are tested. PASS does not mean the entire console RAM or all hardware passed. No saves are changed.

## Automatic runtime tour

Clean before changing build flags, then build with `TOUR=1` and recreate the CUE. The tour runs three buffer checks, one ping and two module reloads, retaining any failure before displaying AUTO TOUR PASS. It does not consume gamepad input and therefore is not an input test.

To restore manual controls, clean and rebuild with `TOUR=0` (the default). Build to a separate directory and reload a fresh disc; do not overwrite an image ares is still using.

For EU/PAL use `REGION=EU VIDEO=PAL`; Japan uses `REGION=JP VIDEO=NTSC`, with a matching BIOS and a clean rebuild. Default is US/NTSC. CPU-only CI covers all three region builds, US debug, US automatic tour and guest-polling-disabled builds. Regional builds are not regional real-console boot tests.

The Lab intentionally leaves 1M-bank switching, ASIC rendering, PCM/CD-DA audio, save media and drive-error recovery to focused system tests. Its existing BIOS and ownership polling helpers are not all bounded by the runtime command timeout.

## Mega EverDrive Pro and capture

The separately installed `megadev-project` skill supplies `scripts/everdrive.py`:
deploy `hardware.iso` (it generates the CUE) or `hardware.cue` plus all tracks.
Hard reset is the default; soft requires an explicit user request. This Lab
does not use SGDK's cartridge `main(hard_reset)` flag or infer a console model.

The updated Pro firmware retained USB during CD execution and passed repeated
live handshakes plus replacement of a running disc with hard reset. The initial
MEGAOS 4.20 setup lost USB during CD/BIOS startup and required menu return before
another upload. Check the actual firmware/setup. The configured BIOS still
needs controller Start. EDIO DETECTED only reports the signature/strobe at A130D4.

`USB_LAB=1` (default) runs a bounded idle-only nonce/CRC handshake and displays
NOT RUN, PASS/LIVE (five-second renewed lease) or PASS/IDLE (last successful
check, host no longer renewing). These are host-check states, not cable detection. `USB_LAB=0` disables FIFO access and displays
USB UPLOAD / MENU MODE for unsupported setups. The link neither initializes disk
access nor competes with a live Main/Sub job. CD/audio streaming coexistence
needs separate validation.

`megadev-testing` supplies direct capture through host FFmpeg/AVFoundation.
Record the build ID and inspect multiple frames, including counters/heartbeat.
The Pro emulates the CD add-on in FPGA; these checks do not qualify an optical
drive or physical Sega CD. No BIOS, saves, audio or camera settings are modified.

Resident budgets remain Main code/rodata FF0000..FF9FFF (40 KiB), BSS
FFA000..FFA7FF (2 KiB), stack FFD000..FFDFFF (4 KiB), separate from BIOS workspace.
Tiles occupy VRAM 0000..1E7F (244 tiles); plane A C000..CFFF, plane B E000..EFFF,
SAT F800..FA7F and hscroll FC00..FC03. Static redraw blanks the display; dynamic
text writes at most 40 words after the existing BIOS VBlank wait, with interrupts
briefly masked around VDP address/data writes. There is no new ISR or frame clock.

## USB-controlled input test

The readiness ROM includes a local copy of the testing skill's optional `UIV1`
hook. A detected EverDrive interface enables it; without detection there is no
FIFO access and input follows the existing physical sampler. The normal sampler
still runs during overrides. Two logical slots independently accept a full pad
or Mega Mouse (left/right/middle/Start, relative X/Y). Held overrides expire in
at most 120 actual VInts; mouse deltas apply once. Explicit release or expiry
returns to the current physical state. No saves or device configuration are
changed by the hook itself; normal game actions still respond to mapped buttons.

Install the testing skill's host pySerial requirements, then use its helper:

```sh
python /path/to/megadev-testing/scripts/usb_input.py --port ACTUAL_PORT --pad1 UP+B+X --pad2 DOWN+C+Z --seconds 5
python /path/to/megadev-testing/scripts/usb_input.py --port ACTUAL_PORT --mouse1=20,8,left --mouse2=-16,-7,right --seconds 5
python /path/to/megadev-testing/scripts/usb_input.py --port ACTUAL_PORT --release
```

The input helper and readiness verifier share one multiplexed FIFO owner. Do not
run two serial processes concurrently. `HLV1` is the separate readiness check;
`UIV1` controls logical input. Both use the existing frame counter. The source
files copied into this Lab are tested against the testing skill's reusable
snapshot, without requiring that skill at build/runtime.

After the BIOS's physical Start prompt, Start on readiness opens Input Lab;
Start returns. That view shows both slots' source, held/pressed/released masks,
per-frame DX/DY, pointer XY and two independent tracks. The physical baseline is
the BIOS's two three-button hold fields; this does not add native six-button or
mouse support to the BIOS. Remote inputs are full logical states. Incoming USB
is deferred during Main/Sub jobs, while lease expiry continues. USB_LAB=0 turns
off both guest protocols and retains the physical sampler. The module and state
remain resident while the Word RAM room runs. No second clock/ISR is installed.

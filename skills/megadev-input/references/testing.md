# Testing and evidence

## Reproducible host and target checks

The repository tests compile the standalone model/transport at O0/O3 and use undefined-behavior checking where supported. They cover:

- All 4096 six-button masks and all low-byte pad masks; polarity and upper-button mapping.
- Mouse sign/zero handling across 256 byte values and four sign combinations, screen Y, buttons, overflow and invalid nibbles.
- All 256 combinations of Team Player device types; mixed payload offsets, empty sockets, unknown types, malformed/truncated packets and per-mouse overflow.
- Baselines, press/release edges, disconnect/recovery, cursor clamps, opposing-direction observation, PAL/NTSC menu repeat, joining and stable assignments.
- Actual protocol writes and read sequences; mouse captures only one falling-TH transaction, all nine mouse timeout positions, Team Player header failure and whole-packet work exhaustion, EA shared TH phase order/selectors and distinct player values.

Input Lab has six target cases per architecture: US/JP/EU release, US debug, a US scripted tour and an explicit US EA build. `tests/integration.py --skill megadev-input` runs actual compiled original-68000 readers with scripted MMIO. It checks direct pad/mouse input, mixed Team Player data, failure cleanup, distinct EA slots, a bounded Z80 grant failure, retained grants and restored interrupt masks. The real UI commit path executes with captured VDP writes and rejects invalid offsets before changing the display cache.

The custom interrupt executes through its register restore with a mocked BIOS INT2 pulse. Unicorn's original-68000 backend raises on RTE, so the CPU test stops at the real RTE boundary, checks the opcode, preserved registers and untouched exception frame. It does **not** claim to execute the exception return or simulate BIOS/drive/VDP/peripheral timing. Full interrupt return belongs to emulator/console integration checks.

CI compares native ARM64/AMD64 payloads and startup/model facts, rejects missing EA/tour/target-execution evidence and runs distribution with telemetry disabled. Local testing can emulate the other container architecture; label that accurately.

## Ares device checks

The audited ares v148 [controller menu](https://github.com/ares-emulator/ares/blob/v148/ares/md/controller/port.cpp) offers Control Pad, Fighting Pad and Mega Mouse. Mega CD has its own mappings, separate from Mega Drive. A keyboard binding does not select the emulated device, and selecting Fighting Pad does not fill its extra-button mappings.

Try the normal build with port 1 left usable for inspector controls. On port 2:

1. Control Pad: exercise directions, A/B/C/Start and press/hold/release.
2. Fighting Pad: confirm PAD6, then map/test X/Y/Z/Mode. Do not require Mode held during boot.
3. Mega Mouse: assign relative X/Y and optional buttons, confirm MOUSE, then inspect dx/dy, cursor direction, clicks and idle state.
4. Nothing/replacement: confirm old held state clears and a replacement establishes a baseline.

Use actual physical input if ares's binding capture ignores or misidentifies synthetic events. Do not overwrite a user's existing mappings merely to make an automation pass. Restore temporary device selections or leave the agreed review setup clearly described.

No multitap option is present in that version. Scripted Team Player/EA results are useful protocol/model evidence, not an ares adapter test. The labeled demo continues running after AUTO PASS; it does not leave a silent frozen screen.

## Physical qualification

Before claiming hardware-ready support, test official and compatible 3/6-button pads, Mode compatibility, Mega Mouse/Sega Mouse, Team Player MULTI and EA/EXTRA modes, empty/mixed sockets, both physical ports, two Team Players, unplug/replug and delayed/missing acknowledgments. Check the timeout settings and pin idle/direction state on failure.

Measure the interrupt-masked batch and Z80 bus interval on PAL and NTSC. Check other Z80 sound drivers, PCM/FM/PSG use, CD access and the game's VBlank/raster services. Operation-count bounds and successful packets do not prove microsecond deadlines or harmless audio contention. No physical peripheral/console qualification is claimed by this slice.

# Mega EverDrive Pro disc deployment

This optional host workflow uploads Megadev Sega CD discs to a **Mega EverDrive
Pro**. Use an explicit port and verify device identity; other EverDrives are not
accepted. Python 3.10+ and pySerial are required. Transport derives from
sgdk-skills' MIT helper; all files needed at runtime live in this skill.

## Actual iteration boundary

On the initial MEGAOS 4.20 setup (MCU `0x0112`, hardware `0x0001`, boot
`0x0101`), USB disappeared during CD-core/BIOS startup and returned in the
EverDrive menu. Reconnecting during the disc run did not restore it; disabling
guest polling made no difference. Such a setup needs menu return before upload.

After the user updated to the current firmware, USB remained available during
CD execution (USB VID:PID `38DF:0018`, extended status protocol). The helper
verified repeated live Lab handshakes and successfully hard-reset/replaced a
running disc through `build-deploy`. Prefer this tested updated setup. Verify
port availability on the actual firmware; the exact first release supporting
this behavior is not established, and legacy/extended discovery alone is not a
complete capability test. The published changelog identifies v26.0925 as latest
at investigation time; use its package installation instructions for updates.

The configured US Sega CD BIOS 2.00 also waits for controller Start. Upload,
installation, and sending launch do not establish that application code ran.
No documented BIOS-skip option was found in the pinned official USB tool. Do not
download, distribute, patch or copy a BIOS automatically. A matching, user-supplied
BIOS must already be configured on the cartridge (`MEGA/bios/mcd-us.bin`,
`mcd-eu.bin` or `mcd-jp.bin`, per its manual). Console model is not inferred.

## Build and deploy

On the host, from the installed skill directory:

```sh
python3 -m venv /path/to/usb-venv
/path/to/usb-venv/bin/python -m pip install -r scripts/requirements-usb.txt
/path/to/usb-venv/bin/python -m serial.tools.list_ports -v
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 ewrogers/megadev:latest doctor
/path/to/usb-venv/bin/python scripts/everdrive.py --port /dev/cu.usbmodemEXAMPLE info
/path/to/usb-venv/bin/python scripts/everdrive.py --port /dev/cu.usbmodemEXAMPLE build-deploy /path/to/game my_game.iso
```

Windows venv Python is `usb-venv\Scripts\python.exe`; use an explicit `COM` port.
Linux often uses `/dev/ttyACM0` and needs the user's normal serial-device access.
Physical acceptance covered macOS only; other hosts have host tests, not device
qualification. Native USB and capture belong on the host on macOS/Windows; no
privileged Docker or container USB passthrough is required. All builds use
`--cpus=2`. The helper preserves a failed build's exit code and never opens USB
after a build failure. `--make-arg TOUR=1` can select the Hardware Lab tour.
Use a fresh project or clean first when changing build configuration/flags, as
required by the project's Makefile; this helper does not silently clean a game.

For a disc already built:

```sh
/path/to/usb-venv/bin/python scripts/everdrive.py --port /dev/cu.usbmodemEXAMPLE deploy /path/to/game/my_game.cue
```

Hard reset is the default for every invocation. Use `--reset soft` **only when
the user explicitly asks for it**; never fall back to soft after a hard-reset
failure. The command resets to the EverDrive menu, uploads, installs and starts
the CUE. This is the cartridge's reset operation, not an electrical power cycle
or SGDK's `main(hard_reset)` contract. Megadev's IP/SP/module startup checks prove
initialized data and cleared BSS independently.

## Supported disc subset and safety

- `.iso`: sector-aligned Sega CD ISO9660, wrapped in a generated CUE using
  `FILE "name.iso" BINARY`, `TRACK 01 MODE1/2048`, `INDEX 01 00:00:00`.
- `.cue`: the same data track plus optional separate stereo 16-bit, 44100 Hz PCM
  WAV audio tracks, one file per track, matching `megadev-release` authoring.
  Supported directives: FILE, TRACK, INDEX 00/01, REM. Data starts at zero;
  audio pregap starts at zero and INDEX 01 must leave at least four seconds.
- Raw 2352-byte rips, multi-track BIN files and other CUE directives are rejected
  by this focused adapter, even though the EverDrive supports more formats.
  Use `megadev-release` for full release qualification; upload preflight is not
  a security-sector, regional BIOS or whole-filesystem validator.

All dependencies are checked and snapshotted into host temporary storage before
opening USB. Missing files, symlinks, path traversal, FAT aliases, unsupported
layouts and truncated PCM fail before reset. Tool limits are 64 KiB CUE, flat
printable ASCII filenames of at most 120 bytes, and 1 GiB total; these are helper
bounds, not hardware limits. Snapshotting uses bounded memory and temporary disk.

Uploads go to `usb-games/<cue-stem>-<bundle-hash>/`, with tracks first and CUE
last. A failed transfer never sends install/start. Existing build directories
are retained; remove obsolete ones through the user's usual SD-card management.
The helper creates `usb-games`; it is not a required firmware folder. All file
hashes, reset choice and destination are emitted as JSON. `runtime_verified`
stays false until separate runtime evidence is inspected.

Serial operations have per-operation and overall deadlines (`--timeout`, default
120 seconds). Increase the overall deadline for large audio discs. Reset release
has a bounded cleanup allowance. Interrupted uploads leave partial files and
require retrying from the menu; there is no resume or automatic reconnect/reset.

## Primary protocol sources

- [Official Pro manual](https://krikzz.com/pub/support/mega-everdrive/pro-series/mega-ed-pro-manual.pdf)
  specifies CUE selection, BIOS paths and FPGA CD emulation.
- [Official firmware history](https://krikzz.com/pub/support/mega-everdrive/pro-series/firmware/changelog.txt):
  MEGAOS V4.02 added MODE1/2048 support.
- [Pinned edlink menu commands](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/MenuCmd.cs)
  install via `*i` and start via `*s`. The upstream one-file Run command does not
  traverse CUE dependencies; this adapter supplies that layer.
- [Pinned transport implementation](https://github.com/krikzz/edlink/tree/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device)
  establishes discovery, reset, file operations and FIFO framing.

The CD core emulates the add-on in FPGA. Passing here is valuable console evidence,
but does not qualify an optical drive, physical Sega CD revisions, seek timing,
CD-DA/PCM coexistence or all BIOSes. Read `megadev-testing` for direct video capture
and `megadev-hardware` for Lab checks and optional guest-link limitations.

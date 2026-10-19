# Mega EverDrive Pro: Sega CD deployment and observability

Investigated 2026-10-19 for Megadev Skills. Source findings are followed by a
separate physical acceptance record; CD-mode host USB verification did not pass
on the initial MEGAOS 4.20 setup.

## Physical acceptance and iteration limit

The native Python helper identified Mega EverDrive Pro over USB, hard-reset to
the menu, uploaded the generated ISO plus MODE1/2048 CUE, installed the CUE and
sent launch twice. Identity reported MCU 0112, hardware 0001, boot 0101; the user
identified the installed OS as 4.20. Base console model remains unspecified.
The configured US Sega CD BIOS identifies itself as 1993 SEGA Ver. 2.00 and
requires controller Start. No BIOS or firmware was uploaded by the helper.

Direct Elgato HD60 X/AVFoundation video showed the Lab boot, Main/Sub C startup
PASS, all four reserved-buffer checks PASS, three RAM checks, two module reloads,
AUTO TOUR PASS, EDIO DETECTED, advancing Main/Sub counters and a blinking RUNNING
indicator. The second disc build displayed ID `733F0007`; its ISO SHA-256 was
`a375e8e43c4e1000739b9f321f18913592bfbfd67e8b3b1e334e32c82d90c509`.
These observations concern the EverDrive FPGA CD core, not a physical Sega CD.

Host USB disappeared during CD/BIOS startup and did not return when the user
replugged the cable. It returned when the user returned to the EverDrive ROM
menu. Repeating with guest FIFO polling disabled reproduced the disappearance
before the Lab entered. That initial setup therefore required menu return
before the next upload and Start at this BIOS prompt. Fully unattended running-
disc replacement and a live guest USB exchange were not established on 4.20.
`USB_LAB=0` provides a no-FIFO fallback displaying USB UPLOAD / MENU MODE.

The current changelog lists v26.0925, with no
explicit promise of CD-mode host USB in its published entries. Published FPGA
source shows CD-specific MCU mode/reset signaling, but the MCU implementation
is not present and these signals do not establish a universal USB policy.
The official Mode-A option bypasses the cartridge menu at power-up, not a
documented Sega CD BIOS Start prompt. Pinned edlink AppStart sends only `*s`.

### Retest after the user's firmware update

The user updated from 4.20 to the current firmware package. The host now observed
USB VID:PID `38DF:0018`, protocol 05/device 18 and extended status discovery.
The helper hard-reset/uploaded/started the CUE successfully, and the USB device
stayed enumerated throughout BIOS startup and Lab execution. Repeated idle
handshakes passed; an eight-second run completed seven fresh exchanges, with
VERIFIED visible on direct HD60 X frames alongside all PASS rows and moving
counters/heartbeat. The running disc was then replaced using `build-deploy`
without manually returning to the menu. The user pressed Start at the BIOS.

Replacement build `01768DE5`, ISO SHA-256
`d2889c426bbac3ecb032a229c50f85bdc787e36ab9915067bcbe17a13fab2453`,
again showed three RAM checks, two module reloads, AUTO TOUR PASS, EDIO DETECTED,
VERIFIED, advancing Main/Sub counters and a blinking heartbeat. A second
eight-second verification completed seven exchanges. The helper still emits
`runtime_verified: false`; these claims come from independently inspected
capture frames and successful protocol exchanges, not an uploader exit code.

The default Lab now enables the bounded guest link (`USB_LAB=1`). This qualifies
the updated tested setup, not every firmware release or USB command during
active streaming/audio. The exact first firmware release with this USB behavior
is unestablished. The whole iteration loop still requires BIOS Start; no BIOS
was copied, patched or downloaded by the integration.

The final UI-only follow-up, build `FAAC2829` (ISO SHA-256
`bf03a531c58a1e2e5337bd2bc403fcd17f0728d085e1313d498d5a645abdb1e3`),
preserves the successful result as PASS/IDLE after the five-second live lease.
Six exchanges over six seconds showed PASS/LIVE; a later direct capture showed
PASS/IDLE with the heartbeat/counters still moving. This avoids presenting a
stopped host verification session as a failed readiness check. NOT RUN precedes
the first successful exchange; live verification still requires renewal.

## Supported minimum workflow

The Mega EverDrive Pro has an FPGA Mega-CD core. A Genesis-compatible console
with the cartridge can execute Sega CD images without a physical Sega CD. A
Megadev release ISO plus its generated single-track CUE is a suitable minimum
deployment artifact. Launch the **CUE**, preserving the associated ISO filename
and directory. This executes the cartridge's CD core, not a physical Sega CD.

The [official manual, Mega-CD core section](https://krikzz.com/pub/support/mega-everdrive/pro-series/mega-ed-pro-manual.pdf)
lists BIN+CUE, CUE+WAV and multi-BIN images, allows IMG/ISO extensions for image
files, and asks for separate directories per image. Its file manager lists CUE
as a launchable extension. Separately, the [official firmware changelog](https://krikzz.com/pub/support/mega-everdrive/pro-series/firmware/changelog.txt)
explicitly adds MODE1/2048 data tracks in MEGAOS-V4.02, dated 2020-07-25. Therefore
the ordinary 2048-byte-sector Megadev ISO need not be converted into 2352-byte
raw sectors. A CUE with a BINARY FILE, TRACK 01 MODE1/2048 and INDEX 01 00:00:00
describes the minimum case. Matching BIOS and console region are still required.
This historical format-support version is not a claimed minimum version for all
current USB commands or firmware status formats.

## Host deployment protocol

Use the same published hard-reset/menu/install/start transport as the cartridge
workflow. The current official [Mega Run implementation](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/DeviceCmd.cs#L58-L66)
performs ResetToMenu, AppDeploy, AppInstall and AppStart without selecting a
different host protocol for a CUE. The menu interprets the file being installed.

The [official menu command implementation](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/DEV_MEGA/MenuCmd.cs#L35-L75)
asserts reset, clears cartridge configuration, releases reset and waits for `r`.
Installation writes `*i` plus the length-prefixed SD path, requires a zero
response, then startup sends `*s`. Hard reset is cartridge pin B2; soft reset is
B27, described as the console reset button in the [official reset examples](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/usb-cmd/README.md).
Hard reset is the default; soft requires an explicit user request. This is a
reset signal, not an electrical power interruption or a command to erase RAM.

**The existing one-file uploader is insufficient for local CUE deployment.**
Official [AppDeploy](https://github.com/krikzz/edlink/blob/064874e2c89df6e8a8cf6dbd3e57661961374d46/edlink/Device/DeviceCmd.cs#L195-L222)
copies only the requested file to `usb-games`, then returns its path. It neither
parses CUE FILE statements nor copies their dependencies. Our independent
adapter must add a disc-set layer above the common USB client:

1. Validate the entire local disc set before opening serial or resetting. Start
   with the generated single-data-track MODE1/2048 CUE/ISO contract. Multi-track
   media is supported by the cartridge, but must be explicitly implemented and
   tested before our helper claims it.
2. Reject missing files, unsupported CUE features, absolute/traversing file paths,
   duplicate ambiguous names and an invalid Megadev ISO. Preserve quoted names
   and spaces with argument lists rather than shell reconstruction. Never
   interpret CUE paths as commands.
3. Verify the selected cartridge identity. Hard-reset to its menu, then copy
   every validated track into a dedicated SD directory and install its CUE only
   after all transfers succeed. A unique build directory avoids launching a CUE
   that references half-replaced files from another build; report the destination.
4. Do not delete previous builds or save data during deployment. Do not silently
   fall back to soft reset. A transfer or installation failure must not send
   AppStart.
5. Report installation and launch transmission separately from observed runtime.
   Capture the video and perform a game-level challenge/response before recording
   those capabilities as verified. Do not append an unexamined post-launch reset.

Edlink's default reset mode is soft, so blindly inheriting its defaults would
violate our hard-reset policy. Its errors also do not provide a sufficient reason
to infer game execution from a successful host process. The source is a protocol
reference, not a requirement to bundle Mono or the official executable.

## BIOS, region and add-ons

The [manual's BIOS instructions](https://krikzz.com/pub/support/mega-everdrive/pro-series/mega-ed-pro-manual.pdf)
require user-provided files `MEGA/bios/mcd-us.bin`, `mcd-jp.bin` and `mcd-eu.bin`
for their respective regions. An image-local `bios.bin` overrides the normal
BIOS. The helper must not download, bundle, copy, overwrite or extract BIOS files
automatically. Keep them and commercial disc data out of Git and reports. An
existing user-supplied BIOS configuration is a prerequisite; a clear installation
error is preferable to silently changing it. A full global three-region setup is
documented by the vendor; a single regional test should not claim to have checked
all three BIOS files.

The manual warns that the CD core cannot be combined with a 32X, and that its
registers conflict with an attached external Mega-CD. Leaving the external drive
unpowered is not a fix. Do not bypass the cartridge's warning setting as an
automated workaround. Nomad and Genesis 3 lack the audio input lines used by the
CD core without modification. PAL/NTSC image mismatch can cause glitches.

The manual's separate CD BIOS-swap/RAM-cart feature for an **attached** Mega-CD
does not turn USB deployment into physical-drive disc emulation. Neither do the
terms "real console" and "hardware capture" establish that the Sega CD ASIC,
sub-CPU, PCM or disc drive are original hardware. The report should distinguish
console identity (unknown unless supplied), cartridge identity, and the CD core
used. Do not infer an Analogue Mega Sg from successful EverDrive detection.

## Runtime EverDrive and USB checks in a CD program

The published [base I/O FPGA module](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/base_io.sv#L32-L65)
decodes `$A130D0` through `$A130DF` on the **main CPU** and provides a SYSSTAT
signature with upper bits `$55A`. A bounded, volatile 16-bit read of `$A130D4`
whose `(value & $FFF0) == $55A0` establishes the EDIO interface. It is capability
detection, not a cryptographically authentic product ID or proof of the console
model. A distinct device-info response is needed before displaying a model name.

The [outer FPGA module](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/everdrive.sv#L250-L268)
instantiates base I/O independently of its
[Mega-CD core](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_base/everdrive.sv#L440-L494).
Its base decode has no `mcd_act` exclusion. **Inference from this source:** the
main CPU's EDIO signature, FIFO and mailbox remain addressable while the CD core
runs. This does not establish that every MCU command is safe or timely during
CD streaming; that is a separate hardware test. The sub-CPU should communicate
through the application's main/sub contract rather than use the main CPU's I/O
addresses directly.

The [official register definitions](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.h#L63-L110)
provide FIFO data `$A130D1`, available-count/status `$A130D2`, and a 16-bit mailbox
`$A130DE`, mirrored to the host at FCI `$01800300`. Host FIFO writes address
FCI `$01810000`. The [guest USB command](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c#L172-L177)
is framed command `$22` plus a big-endian 16-bit length and payload. The official
sample is a cartridge program, so the sample alone is not CD-mode USB evidence.

Use a small, bounded challenge/response serviced by the main loop, with a fresh
nonce/build identity and an expiring verification lease. It can reuse the proven
SGDK helper's **wire format**, adapted to Megadev's main CPU and existing clock;
do not copy SGDK runtime APIs. Probe the status signature before writes, poll
available bytes without blocking, bound parser storage/traffic, and show absence
as unknown/unavailable. Never mark a link verified simply because EDIO was found
or the host installed a disc. Report WAITING, VERIFIED and EXPIRED accurately;
an expired host lease does not mean the cartridge or running disc failed.

Do not import the complete blocking `ed_init()` routine into a game: the sample
[initialization](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/edio/everdrive.c#L85-L102)
flushes FIFO and initializes disk access. The cartridge MCU also services the
[CD core](https://github.com/krikzz/mega-ed-pub/blob/23dbd5ef527999eb85590ece7ad0ba6e202c8a4c/fpga/mapper/lib_mcd/megacd_top.sv#L56-L58),
so boot readiness should avoid extra SD/disk-init or mount commands. Verify an
idle-disc handshake first, then deliberately exercise it alongside CD reads and
audio, watching for missed frames or stalled sub-CPU work. If MCU-mediated USB
commands prove disruptive, the shared mailbox offers a smaller direct register
roundtrip to investigate; it still requires its own concurrency/hardware proof.

## What the Megadev readiness disc should establish

- Build label and actual region; moving main CPU heartbeat using the existing
  frame clock; initial DATA/BSS values captured before mutation.
- A boot-session/replacement marker, rather than SGDK's `main(hard_reset)` flag.
  A Sega CD BIOS/IP/SP boot sequence is not an SGDK cartridge entry path. Do not
  infer a true RAM clear from the host's reset-mode argument.
- A bounded main/sub request and response with distinct result/timeout states,
  and a disc fixture read whose contents are checked. These provide evidence
  beyond successfully rendering an IP-resident screen.
- Word RAM ownership checks only through verified Megadev contracts; PCM/CDDA
  playback as opt-in audible/capture evidence; no automatic BRAM format or writes.
- EDIO presence and a fresh USB host response independently. Keep the console
  model unknown and label the CD implementation accurately.

## Source versions and reproducibility

- Official `krikzz/edlink`: commit
  `064874e2c89df6e8a8cf6dbd3e57661961374d46` (2026-10-14).
- Official `krikzz/mega-ed-pub`: commit
  `23dbd5ef527999eb85590ece7ad0ba6e202c8a4c` (2026-06-08).
- Krikzz's live manual fetched 2026-10-19, SHA-256
  `c0fc2133cb2258ab80838e7668ef973bdfda3295d4d1196c8e4242bb0ba8704c`.
- Live firmware changelog fetched the same day, SHA-256
  `414232c07776b269b3e87c285a142ebd9a734abebe0da7c1b00dcef046054658`;
  newest entry in this snapshot is `mega-pro-fw-v26.0925`.

Live manual/changelog URLs are not immutable releases. Downloads and extracted
research intermediates remain under ignored `build/everdrive-research/`; they
are not installed-skill dependencies or redistributable BIOS assets.

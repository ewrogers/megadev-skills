# Optional USB input validation — 2026-10-19

The testing skill owns UIV1, the reusable C module, installer and Python client.
Input retains physical-driver/game snapshot ownership; hardware supplies an
independently buildable adapter; project retains deployment/reset. Main state
stays resident, and CD jobs defer FIFO polling without stopping lease expiry.
Only cooperating development ROMs include this optional hook.

## Build and contract evidence

Pulled `ewrogers/megadev:latest`: Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0, binutils
2.47.20260726, genisoimage 1.1.11. Manifest
`ebf70a3984a12875c68ff4a12dbb3c2799c30008e5ea81bbff689d4f0d0a881f`.
Public commands retain the mutable tag; every Docker run used `--cpus=2`.

All 138 host tests passed, including seven new USB tests. O0/O3 C contracts use
UBSan; fragmented transcript tests cover legacy/extended identity, noise,
stale/bad-CRC ACKs, rejected requests and fresh session nonces. An independently
copied skill installs sources without overwrite. Telemetry-disabled CLI
installation of all 21 skills passed; changed skills passed quick validation.

Hardware target matrices passed on ARM64 and locally emulated AMD64: US/JP/EU,
release/debug, auto-tour and USB disabled. Complete payload/startup reports
matched across architectures. Original-68000 execution covers startup, Main/Sub
probes and the multi-packet UIV1 suite: atomic independent slots, mixed device
kinds, one-shot motion, all four mouse buttons, edges, release/expiry, invalid
requests, CRC, stale sequence, HLV1 multiplexing, partial timeout and VInt wrap.
Absent-interface execution touches no FIFO MMIO. Native dual-architecture CI
remains separate evidence; local AMD64 used Docker emulation.

A transient Docker future-mtime warning aborted the first run. Debug execution
needed a larger instruction termination guard for the full packet suite; the
USB-disabled build also caught an unused callback, now compiled conditionally.
The completed matrices passed without those warnings. CPU instruction guards
are not cycle measurements; fixed protocol state is at most 128 bytes.

## Physical observation

Mac host: pySerial 3.5, FFmpeg 9.0.2; EverDrive Pro VID:PID 38DF:0018,
extended protocol 05/device 18, firmware updated by the user earlier. No console
model is inferred. The release disc was deployed with the default hard reset:

- Build: `744207F6`.
- ISO SHA-256: `da8f2f39042067b0bea40649a67156dca82b3a5aae64b37906769c225a77e6f0`.
- Evidence: ignored `build/usb-input-final-ready`,
  `build/usb-input-final2-capture/report.json` and `build/usb-input-final-cli.json`.

Direct HD60 X frames showed Main/Sub startup PASS, EDIO DETECTED and an advancing
heartbeat. After physical BIOS Start, USB navigated to Input Lab. Captures
confirmed pad masks 0411/0122, mouse on either slot, both mice, all four mouse
button bits and mixed pad/mouse input. Pointer positions changed once per delta;
renewals carried zero deltas. Explicit release and an unrenewed 30-VInt pulse
returned to BIOS physical input. A legacy readiness handshake completed while a
UIV1 pad override stayed active. The standalone installed-style Python transport
and CLI also identified the Pro and injected port-2 Z successfully.
USB C then loaded the Word RAM disc module; fresh frames showed DATA/BSS PASS.
USB A returned to readiness and Start reopened Input Lab, ending with no active
overrides. Evidence: `build/usb-input-final-disc-navigation-v2`.

An earlier two-second ACK deadline timed out during capture; repeated tests with
a configurable bounded five-second limit passed. Measured unloaded round trips
were approximately 0.44 seconds on this view. This does not establish the exact
cause of the earlier delay or guarantee gameplay/HDMI input latency.

Stock BIOS Start remains physical. These observations qualify cooperative
logical input on this updated EverDrive CD core, not native mouse/multitap
hardware, a physical Sega CD/optical drive, stressed streaming/audio, saves,
other firmware, electrical power cycling or arbitrary-ROM control.

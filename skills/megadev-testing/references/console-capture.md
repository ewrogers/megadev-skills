# Observe a console through a capture card

`scripts/capture_console.py` is a self-contained Python standard-library wrapper
around host FFmpeg. The current backend is **macOS AVFoundation video only**.
Use capture when the user has authorized hardware observation; keep each capture
bounded and inspect its actual frames before claiming a runtime result.

```sh
ffmpeg -hide_banner -f avfoundation -list_devices true -i ""
python3 scripts/capture_console.py --device "Elgato HD60 X" --output /path/to/new-capture --frames 4 --sample-fps 2 --after 2
```

Requires FFmpeg on PATH (or `--ffmpeg /absolute/path/to/ffmpeg`) and macOS Camera
permission for the executing host process. No Docker, window capture or Elgato
Studio automation is involved. Select an exact device name, not a transient index.
The device-listing command prints to stderr and normally exits nonzero without
an input selection. The tested HD60 X uses 1920x1080, 30 fps, UYVY422. Inspect `--help` for overrides
and supported ranges; audio is explicitly disabled.

The destination must be new. The helper emits PNG frames, `capture.log` and
`capture.json` with device, requested settings, FFmpeg command/version, image
hashes and timings. It limits frame counts/timeouts and refuses overwrite. A
successful capture reports `runtime_verified: false`: a frame existing is not
proof of the application state. Read the visible build ID, PASS/FAIL rows and
at least two different heartbeat/counter states. A single static screenshot
cannot distinguish an active game from a freeze. Capture delay and sampling
are host timings, not measured console VBlank cadence or input latency.

Direct HD60 X capture coexisted with the user's Elgato Studio preview on the
tested Mac. That observation is not a general guarantee of multi-application
sharing. If the device is busy, report it; do not close the user's preview,
change its settings or substitute a window screenshot without authorization.
Other capture devices and Linux/Windows backends are not physically qualified.

For the Megadev Hardware Lab, record the disc/ISO hash, build ID, region, BIOS,
EverDrive identity and whether Start/menu-return required user input. Its tour
checks Main/Sub C startup, reserved scratch buffers, 2M Word RAM exchange, ping
and two module loads. CD-DA, PCM, 1M mode, ASIC, saves, drive-error recovery and
controller mapping need their own focused tests. EverDrive CD-core observations
do not establish optical drive or physical Sega CD behavior.

Responsibilities: `megadev-project` owns disc build/upload/reset;
`megadev-hardware` owns register contracts and the diagnostic fixture;
this skill owns capture artifacts and evidence. The helper derives from the
MIT sgdk-skills capture helper and needs no sibling skill at runtime.

# Math Lab

A small Megadev disc showing fixed-point orbiting dots, normalized target pursuit, and linear versus eased interpolation. Its bright markers and motion are illustrations; the reusable part is the tested numeric code.

## Build

Create a fresh project with the skill's `scripts/create_project.py` first. From the generated project folder, with Docker running:

```sh
docker run --rm --cpus=2 --platform linux/arm64 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --platform linux/arm64 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue math.iso
```

Use `linux/amd64` on an x64 machine. In PowerShell use `${PWD}` for the source path. The image contains Megadev and the original-M68000 compiler; no SGDK installation is used. A normal build selects US/NTSC. Pass `REGION=EU VIDEO=PAL` to build a European disc or `REGION=JP VIDEO=NTSC` for Japan. Use a fresh project or clean build when changing region. `TOUR=1` runs a deterministic correctness tour and ends paused at AUTO PASS; it is not a performance mode.

Open `math.cue` in ares using its **Mega CD** system and your own configured matching BIOS. With its command line, specify `--system 'Mega CD'` rather than relying on auto-detection.

## Controls

| Control | Action |
| --- | --- |
| C | Next view: Trig / Vectors / Interpolation |
| B | Reset current view |
| Start | Pause/resume |
| Up/Down | Change tick speed in Trig/Interpolation |
| A | Reverse orbit/easing; toggle automatic vector target |
| D-pad in Vectors | Move the gold target, disabling its automatic path |

Trig draws each position from a fixed base vector and a phase, so repeated rotations do not accumulate rounding drift. In Vectors, the blue point follows the gold target; purple dots show the interpolated segment. Interpolation compares blue linear motion to gold smoothstep over the same endpoints and duration. The top HUD identifies the current view and region. B has a short PSG cue; no music is required.

## Executable checks

With Python 3 and a host C17 compiler:

```sh
python tools/check_math.py
```

This compiles the **same** numeric/motion C sources at O0/O3. The repository adds independent randomized oracles and compiled original-M68000 execution on both container architectures, including US/JP/EU/debug/tour builds. Host C alone does not exercise the target inline assembly.

`src/math_core.h` documents the application API. Signed Q8.8 only spans −128..127.996 pixels; these are offsets around a separate screen center, not general world coordinates. Checked outputs preserve their prior value on overflow/invalid inputs. `mm_length2` requires a non-null vector. The SDK's conversion/division wrappers at the audited revision are unsuitable; see the skill's arithmetic reference for the exact findings. Only its qualified `mul/mulu` helpers are used here.

No heap allocations, worker service, CD streaming or hardware benchmark is implied. The room fixture and Sub startup verify standalone modules; all demo arithmetic runs on Main. The renderer has bounded immutable publication and only installs palettes while display is off. Emulator and instruction tests do not replace real-hardware timing checks.

Art, table and font are generated locally by the included Python tools. `make_art.py` regenerates the mathematical quarter-wave table using host floating point; target code is integer-only. Licenses are in `LICENSE` and `LICENSE.megadev`.

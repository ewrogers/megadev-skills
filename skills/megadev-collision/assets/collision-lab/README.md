# Collision Lab

Three small views show swept boxes, finite rays and wall sliding. The cool grid,
colored markers, sounds and controls are illustrative. Use the techniques in your
own game's coordinate system and existing runtime.

Create a fresh project from the installed skill:

```sh
python /path/to/megadev-collision/scripts/create_project.py ./collision-game
cd collision-game
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue collision.iso
```

PowerShell: use `${PWD}` instead of `$PWD` in the mount. Docker must run Linux
containers. Both `linux/arm64` and `linux/amd64` are supported. The image supplies
Megadev and the original-68000 compiler defaults; no SGDK installation is needed.

On macOS, launch with the system specified so ares does not ask which console:

```sh
open -a /Applications/ares.app --args --system 'Mega CD' "$PWD/collision.cue"
```

Configure your own regional BIOS in ares. Press Start at its boot screen if needed.
The usual review bindings are A=J, B=K, C=L, Start=Enter, D-pad=arrows.

| View | Try this |
| --- | --- |
| Shots | A fires/resets. B changes 8/64/128 pixels per step. Up raises/lowers the wall. Pink uses endpoint overlap and tunnels at high speed; blue sweeps and stops against the 4-pixel wall. |
| Rays | D-pad moves the crosshair; A resets. The dotted segment ends at its first wall. Gold marks the hit; an arrow shows the blocked axis. A ray along an edge does not enter it. |
| Sliding | D-pad moves the rover; B changes speed; A resets. Try fast diagonal movement into the L-shaped obstacles and corners. The rover's visible body matches its 12×12 collision box. |

C cycles views and Start pauses. Shots stop until you fire again. The HUD shows controls and contact status. `CcStats` exposes candidate/narrow-test
counts for instrumentation; these are not FPS or CPU-load figures.
Diagonal movement is intentionally unnormalized to stress collision.

For a PAL build, clean first, then build with `REGION=EU VIDEO=PAL` and regenerate
the CUE. Use a European BIOS. `TOUR=1` runs a short automatic correctness tour;
it is not a performance benchmark.

## Checks

```sh
python tools/check_collision.py
```

This runs the portable geometry/controller contracts at O0/O3 with undefined-
behavior sanitization where supported. The repository adds 12,000 exact-fraction
oracle comparisons, original-68000 execution, interrupt/publication checks,
regional/debug/tour builds and architecture payload comparison. See the skill's
validation reference for evidence and limits. No BIOS or build images are shipped.

`res/layouts.json` is the shared source for visible wall tiles and geometry;
`tools/make_art.py` creates native 8×8 patterns, maps, palettes and the generated
layout header. `tools/make_font.py` creates the font. Both are deterministic.
Keep generated `build/` files out of source control.

The prototype supports axis-aligned boxes and static-wall sliding, not slopes,
rotating shapes, platform pushing or a full physics solver. Use explicit recovery
for initial overlaps, invalid geometry and any unresolved motion.

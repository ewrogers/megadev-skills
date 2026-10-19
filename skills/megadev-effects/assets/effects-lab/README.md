# Effects Lab — Winter Observatory

A small native-pixel observatory demonstrates bounded sparks, smoke, trails,
rain, snow and presentation-only shake. The generated art, quiet PSG notes,
controls and tuning are illustrative. Reuse the techniques with your game's
own style and existing renderer/audio owner.

## Build and open

Generate a new project using `scripts/create_project.py`, then in that folder:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue effects.iso
```

Use the chosen ID if different. Keep the CUE beside its ISO. For PAL, build a
fresh generated project with `REGION=EU VIDEO=PAL`. Select
`--platform linux/arm64` or `--platform linux/amd64` for architecture checks.
Use an absolute mount path on Windows. Unix users may add
`--user "$(id -u):$(id -g)"` for host-owned output.

On macOS, with your own configured BIOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/effects.cue"
```

## Controls

- D-pad moves the lantern; its trail fades behind it.
- A creates a spark/smoke burst and a finite sound. Hold does not repeat.
- B cycles clear, rain and snow.
- C selects weather density 1/4 through 4/4.
- Start pauses; Up+Start toggles shake.

Typical ares bindings are J/K/L for A/B/C, Enter for Start. FPS counts published
frames (normally 60 NTSC or 50 PAL). FX is submitted sprites including the
lantern. SKIP counts live effects omitted from that frame's sprite budget;
those particles still simulate. A deliberately crowded band can skip weather
without a game failure. These are presentation limits, not simulation capacity.

## Budgets and checks

The 64-particle pool reserves 24 impact/smoke, 8 trail and 32 weather slots.
The list submits at most 56 hardware sprites and conservatively limits each
8-line band. Effect patterns remain resident. There are no steady-state tile
uploads or palette writes, no heap allocation and no decoder/ASIC job.
Foreground prepares immutable frame snapshots; the existing early-VBlank owner
publishes up to 776 data bytes of SAT, scroll and HUD state.

`TOUR=1` runs a model stress check, then combines maximum snow and periodic
bursts. Look for AUTO PASS and a stable regional frame rate. Automated host and
original-68000 tests cover pool exhaustion, timing, culling/scanlines, guards
and publication. Real hardware still needs timing/bus qualification; a byte
bound and an emulator FPS observation do not prove spare CPU time.

`tools/make_art.py` creates crisp original 320×224 indexed scenery and a 13-tile
hero/effect atlas, directly at working resolution. `make_font.py` creates the
readable native font. Neither requires network access or an image service.
Bootstrap code derives from Megadev; retain `LICENSE.megadev`. `fx_*`, the art
and renderer integration are application code, not SDK particle APIs.

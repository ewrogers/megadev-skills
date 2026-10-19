# Combat Lab — Copperleaf Sparring Court

A small original, native-pixel top-down sparring example. Art, music, controls,
opponents and tuning illustrate combat contracts; they are not a game template
that other projects should copy.

- D-pad: move/face; A: sword; hold B: directional guard; C: bolt.
- Start: begin/pause. Second configured pad: Start joins, same A/B/C actions.
- Defeat the three opponents; B restarts after clear or defeat.
- Violet pursues and swings; amber and pale opponents guard periodically, with
  vulnerable gaps. Pale also casts. Watch the ! windup before their swings.
- Active blades reach roughly two tiles forward. A blocked or invulnerable
  contact consumes that swing's hit; friendly fire is off. Mutual active hits trade.

Create a fresh folder with the skill's `scripts/create_project.py`. Then:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 -v "${PWD}:/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 -v "${PWD}:/workspace" ewrogers/megadev:latest cue combat.iso
python3 tools/check_combat.py
```

Use `--mount` with an absolute local path if your shell handles `${PWD}` differently.
`REGION=EU VIDEO=PAL` builds PAL. `TOUR=1` executes the contract before normal play;
it does not inject player input. Ares needs an independently supplied matching
BIOS and the explicit Mega CD system. No BIOS/disc/save file is shipped here.

`combat.c/h` is application code with no hardware access; `ipx.c/bridge.s` owns
Main publication/input, `sound.c` owns this Lab's PSG and Sub keeps BIOS services
alive. Five actors/eight bolts, resident art, finite hit records and four-step
catch-up keep work bounded. Actual target timings still need measurement.

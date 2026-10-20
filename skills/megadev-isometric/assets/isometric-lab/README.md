# Isometric Lab — Lantern Conservatory

A native-pixel floating garden with an elevated bridge, stairs, towers, trees and
an animated lantern keeper. Art, sound, controls and layout are illustrative.

- D-pad walks in screen-relative directions.
- A tours the stair approach, steps, upper deck and underpass.
- B toggles a deck/stair cutaway; collision and the current surface stay unchanged.
- C shows height/order information and a foot marker. Start pauses.

Walk onto a stair from its end; upper edges refuse a step into thin air. You can
also approach the same bridge from below and walk underneath it. Objects are
ordered as logical volumes; tall objects use multiple hardware sprites.

Create a fresh folder with this skill's `scripts/create_project.py`, then:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 -v "${PWD}:/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 -v "${PWD}:/workspace" ewrogers/megadev:latest cue isometric.iso
python3 tools/check_isometric.py
```

Use `--mount` with an absolute path if required by your shell. Build PAL with
`REGION=EU VIDEO=PAL`. `TOUR=1` runs the contract before normal play; it does not
inject controls. Ares requires a separately supplied matching BIOS and the
explicit Mega CD system. BIOS, built discs and saves are not shipped.

`iso.c`, `world.c` and `controller.c` are application models. `ipx.c`/`bridge.s`
own the standalone renderer/input path. Resident art is unpacked once through
Main-owned Word RAM; gameplay does not stream/decode art. The graphics ASIC is
not needed. Reuse the smallest useful technique in your own game rather than
copying the garden's design or installing duplicate services.

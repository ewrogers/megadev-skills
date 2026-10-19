# Enemy Lab — The Moonwatch Court

Collect three blue lenses, then leave through the lit east gate. The violet
guard patrols, the amber hunter pursues, and the pale sentry turns and fires.
Walls hide you. Your entry marker has nearby cover; it is not an invulnerable
zone. Art, music, controls and enemy tuning illustrate the techniques only.

## Build

From the installed skill:

```sh
python scripts/create_project.py enemy-game
cd enemy-game
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue enemy.iso
```

Use `--platform linux/arm64` or `linux/amd64` to choose the host architecture.
For PAL, create a fresh folder and append `REGION=EU VIDEO=PAL` to `build`.
Windows Docker Desktop needs Linux containers and an absolute mount path.
No image package is needed: standard Python generates native pixels directly.

With your own configured BIOS on macOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/enemy.cue"
```

## Controls

- D-pad: move. A: stun nearby enemies if the white recharge bar is full.
- B: restart. C: cycle Normal, Sense and Path views. Start: pause/resume.
- `!` means alert/pursuit; `?` means searching; `zZ` means stunned. The body
  flashes during the final recovery second. Alert, fire and stun have cues.
- Three hearts absorb body/projectile hits, with a short hurt grace period.
  On clear, a panel, fanfare and animated celebration confirm completion.
- Typical keyboard profiles use J/K/L for A/B/C. Check your ares Start binding;
  this is an emulator setting, not a keyboard requirement in the game.

Sense shows bounded samples of each enemy's actual wall-blocked vision, not a
filled cone. Path shows at most eight remaining waypoints per enemy; a direct
route needs no graph search, so it may have no dots. Actors take priority over
diagnostic markers if sprite limits are reached.

## Work and memory bounds

The 60 Hz model uses one shared BFS workspace: 240-byte parents, a 240-byte
queue and 20 bytes of metadata. At most 12 nodes expand per simulation step,
48 per clamped foreground update. Each of three enemies has a 64-byte route;
there are six projectile slots. There is no heap, recursion, Sub AI worker or
gameplay CD load. See `enemy.h` and the link map for the full world/state cost.

The renderer reserves two immutable SAT/HUD snapshots. Native art uses 188
sprite tiles, 24 ground tiles, 96 font tiles and four legal VDP palettes.
Normal VBlank publication is at most 768 data bytes; a changed clear/game-over
panel adds 320 bytes once. Palettes/maps/art are installed with display off.
No per-frame palette or background redraw runs during movement.

`python tools/check_enemy.py` executes C17 O0/O3 behavior and work-bound checks.
`TOUR=1` additionally runs the same contract on the target before normal play.
Repository tests add an independent random-map shortest-path oracle, actual
M68000 execution, VDP publication checks and ARM64/AMD64 regional comparisons.
These tests do not establish console cycle timings, physical sound quality or
the scalability of a larger game. Preserve `LICENSE` and `LICENSE.megadev`.

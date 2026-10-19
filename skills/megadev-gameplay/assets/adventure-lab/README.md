# Adventure Lab — The Glass Garden

A small original two-area Megadev example: explore a garden and lantern court,
collect twelve gems, avoid patrolling beetles and use a rechargeable pulse.
Main keeps movement, collision and camera state responsive while an optional
Sub job processes its own guarded Word RAM buffer.

Art, music, controls, rules and layout are illustrative. This is a gameplay
contract example, not a required framework or visual style for other games.

## Create and build

From the installed skill directory, create a new folder:

```sh
python3 scripts/create_project.py my_adventure
cd my_adventure
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest cue adventure.iso
ares --system "Mega CD" adventure.cue
```

Use an absolute mount source; in PowerShell use `${PWD}`. Docker needs Linux
containers, on ARM64 or AMD64. Supply your own legal BIOS in ares. For PAL, build
a fresh copy with `build REGION=EU VIDEO=PAL`. Do not rewrite a loaded disc.

## Play

| Pad | Action |
| --- | --- |
| D-pad | Walk; the camera follows within area bounds |
| A | Pulse: nearby beetles stop for two seconds; recharge takes three |
| B | Reset the expedition, without revoking a live service request |
| C | Start a bounded Sub job while continuing to move |
| Start | Pause/resume gameplay; service observation continues |

Usual local mappings are A=J, B=K, C=L and Start=Enter. Use your configured pad
mappings if different. At the BIOS screen, press Start to boot.

The pale arrow path marks the gate at each side. Walk right to the next area;
walk left through its gate to return. Gems remain collected in both areas.
Three beetle contacts cause a safe-entry respawn; collected gems stay collected.
There is brief protection after a hit. Collect all twelve for the clear cue and
message; movement continues. B starts over.

C uses the unchanged System Lab service: it transforms 256 words in bounded
chunks, verifies the result and retires the request. The HUD returns to READY.
The optional `build TOUR=1` automatically starts a healthy job, crosses areas
and drains a deliberately delayed job; AUTO PASS is a smoke result, not a
performance measurement or exhaustive test. No save media is written.

## Inspect and test

`python3 tools/check_gameplay.py` runs portable C behavior at O0/O3 with a host
compiler. `make_art.py` regenerates original art at its native resolution:
16x24 explorer, 16x16 beetles, 8x8 gems, 32x32 pulse and 8x8 ground patterns.
Four legal VDP palettes separate the UI, actors and two areas. `make_font.py`
regenerates the original crisp 5x7 font in opaque 8x8 HUD cells.

`gameplay.c` owns simulation only. `ipx.c` adapts BIOS input, publishes immutable
frames and keeps service time independent of pause. `coord.c`/`*_link.c` retain
the established Main/Sub owner; `gfx.c` retains conservative sprite admission.
`sound.c` is a small standalone Main-only PSG owner for the Lab's original tune
and finite cues, not a replacement audio driver for an existing game.

No runtime allocation or CD read is required at an area gate. The resident
map is replaced with display disabled; this example does not implement seamless
streaming. ROOM.MMD remains a startup-link fixture and is not executed by the
runtime. Runtime Word RAM belongs to the optional service; renderer/game state
stays in Main Work RAM. Initial disc bootstrap is synchronous.

Repository tests add paired original-68000 execution and matching ARM64/AMD64
payloads. Those tests do not emulate the VDP, BIOS timing, drive or sound. Check
PAL/NTSC presentation, input, audio and frame cadence in ares; raster margin,
stack high-water and combined physical-console workloads remain unqualified.

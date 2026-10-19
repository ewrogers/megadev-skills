# Animation Lab — Clockwork Yard

A standalone Megadev timeline example with a duelist and practice dummy. Named
clips carry frame holds, entry events and active hitboxes. The controller shows
one hit per swing, hurt interruption, a terminal death pose and explicit recovery.

Art, music, controls, combat rules and tuning are illustrative. Reuse the timing
and lifetime contracts rather than this scene's design for an original game.

## Create and build

From the installed skill directory, create a new project:

```sh
python3 scripts/create_project.py my_animation
cd my_animation
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest cue animation.iso
ares --system "Mega CD" animation.cue
```

Use an absolute mount source; `${PWD}` in PowerShell. Docker needs Linux
containers, on ARM64 or AMD64. Supply your own legal BIOS. Use a fresh project
with `build REGION=EU VIDEO=PAL` for PAL; do not rewrite an ares-loaded disc.

## Try it

| Pad | Action |
| --- | --- |
| Left / Right | Walk and face the corresponding direction |
| A | Attack; after death finishes, begin rising |
| B | Interrupt with hurt |
| Down + B | Interrupt with a fall, then hold its final pose |
| C | Toggle actual body/dummy/active sword boxes |
| Up | Toggle quarter-speed inspection |
| Start | Pause/resume |

Usual local mappings are A=J, B=K, C=L, Start=Enter; use your configured mappings
if different. Press Start at the BIOS screen to boot.

Approach the dummy before attacking. Each swing may overlap it on several steps,
but HITS increases once. The counter is hexadecimal. B removes an active sword
immediately; the abandoned swing does not later hit or complete. Death retains
its final pose until a new A press starts the rise. Moving away or holding A
should not create new attacks. Walking and attacks have finite timed sound cues.

Use slow motion and boxes together to inspect active frames. The body, weapon,
damage test and outline all use the same simulation pose. The renderer can skip
poses at display rate; gameplay consumes their events/contacts per logical step.

`build TOUR=1` attacks, interrupts another windup, falls and rises, then pauses
on AUTO PASS. It is a smoke test, not a benchmark or an exhaustive playthrough.
No runtime disc reads, save writes or background animation jobs are required.

## Source and checks

- `animation.c`: immutable named clips and a ten-byte playback state. Frame
  selection and completion have no timer, allocation or renderer dependency.
- `training.c`: one fixed-step clock and illustrative action-priority/hit policy.
- `ipx.c`: BIOS input adapter and immutable SAT/HUD publication through one
  project-owned VBlank path. Animation and collision run in foreground.
- `gfx.c` and `bridge.s`: unchanged conservative sprite admission/IRQ contract
  from the established gameplay example.
- `sound.c`: standalone Main-only PSG music/cues. Existing games keep their audio
  driver/owner rather than installing this convenience player.

Run `python3 tools/check_animation.py` with a host C compiler for O0/O3 behavior
checks. `make_art.py` creates original native 16x24 body/dummy poses, 32x32 sword
arcs, exact hitbox guides and legal palettes. RGB channel packing is explicit;
source bytes regenerate exactly. No resizing, external art generation or tool
installation is required. `make_font.py` generates the crisp 5x7 HUD font.

Repository CI adds original-68000 execution, PAL/NTSC builds and matching native
ARM64/AMD64 payloads. Emulator and physical-console evidence remain separate.
There are no normal-frame CRAM writes; assets/maps are installed with display
disabled. ROOM.MMD remains a module-startup fixture, not a runtime animation load.

Original Lab material is MIT licensed; upstream Megadev bootstrap notices are
preserved in `LICENSE.megadev`. No BIOS or commercial assets are included.

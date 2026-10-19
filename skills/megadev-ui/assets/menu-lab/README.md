# Menu Lab: Wayfarer Desk

Native bitmap fonts, nine-slice panels, pack grid, item details, character view,
settings, pause and a changing HUD. Art, items, controls and flow are illustrative.
No SGDK library or save-data writes are involved.

## Build and run

Create a new folder using the skill's `scripts/create_project.py`, then run from
that folder with Docker using Linux containers:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue menu_lab.iso
ares --system "Mega CD" menu_lab.cue
```

PowerShell uses `${PWD}` in the mount string; CMD uses `%CD%`. ARM64 and AMD64
Linux containers are supported. Ares needs your own regional BIOS; none is
included. Press Start at the BIOS screen if needed. Use `REGION=EU VIDEO=PAL`
for Europe and a compatible BIOS. Rebuild cleanly when changing flags.

## Try it

| Screen | Controls |
| --- | --- |
| Title | D-pad chooses Journey / Pack / Settings / Credits; A or Start opens |
| Journey | D-pad moves; A adds coins; B changes HP; C opens Pack; Start pauses |
| Pack | D-pad selects an item; A opens Details; C opens Character |
| Details / Character | B returns to the same selected item |
| Settings | Up/Down selects; Left/Right or A changes font or cursor pulse |
| Pause | Resume / Pack / Settings / Title; Start resumes immediately |
| Other pages | B returns to the previous page |

Use your configured emulator bindings. J/K/L mean A/B/C only if mapped that way.
Pages build in a hidden tilemap and switch after seven row batches; the small
transition delay is intentional. Digits/cursors should not leave stale cells.
The example is silent; audio remains with the audio system.

## Tests and resources

```sh
python tools/check_ui.py
python tools/make_assets.py
python tools/preview.py preview
```

The executable checks and preview require a host C compiler. Python asset/PNG
encoding uses only its standard library. Preview requires a new output folder.
Art is original code-authored native pixel art, not resized external images.
VRAM and four palette-bank reservations are explicit in the source/generator.

`TOUR=1` walks through the actual input/model path. Repository checks execute
the target publisher and compare ARM64/AMD64 outputs. CPU tests do not emulate
video or measure VBlank timing; emulator/console checks are separate.
The original work is MIT; startup scaffolding retains `LICENSE.megadev` notices.

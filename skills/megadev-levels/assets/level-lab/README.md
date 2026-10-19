# Level Lab

Explore two connected areas built from editable Tiled maps. Ground, foreground,
collision and named interaction objects remain separate. The explorer, orchard,
court, sounds and controls illustrate the pipeline; they are not a game template.

## Build and try

Create a fresh folder using the installed skill:

```sh
python /path/to/megadev-levels/scripts/create_project.py ./level-game
cd level-game
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue levels.iso
```

PowerShell uses `${PWD}` in the mount. Docker must use Linux containers; both
`linux/arm64` and `linux/amd64` work. The checked prepared art is included, so
normal container builds need no Pillow installation or asset download.

On macOS:

```sh
open -a /Applications/ares.app --args --system 'Mega CD' "$PWD/levels.cue"
```

Use your own regional BIOS. Press Start to boot if needed. Review bindings are
A=J, B=K, C=L, Start=Enter and D-pad=arrows.

- D-pad walks. The camera follows both axes; the HUD stays fixed.
- A at a gold waystone travels to the other area. Follow the east/west path.
- B shows the separate collision grid; B returns to artwork.
- C tours five safe spawn/camera positions without changing areas.
- Start pauses movement and camera updates.

Canopies are a foreground layer; try walking below them. Walls and water are
solid independently of the visible tile. Explicit area changes, tour warps and
collision-view changes briefly rebuild the cache while display is disabled;
ordinary walking only uploads incoming rows/columns.

For PAL, clean before rebuilding with `REGION=EU VIDEO=PAL`, regenerate the CUE
and use a European BIOS. `TOUR=1` builds an automatic correctness tour; it is not
a performance benchmark.

## Edit in Tiled

Open `res/orchard.tmj` or `res/archive.tmj` in Tiled. The shared external tileset
is `res/terrain.tsj`, backed by the native 128×64 `terrain.png`. Edit Ground,
Foreground and Collision separately. Objects contains named Spawn points and
Exit rectangles with `target`/`spawn` properties.

After editing, prepare on the host:

```sh
python -m venv .venv
# Activate it using your shell, then:
python -m pip install "Pillow>=10,<14"
python tools/import_tiled.py res/world.json res/world.bin
```

Rebuild through Docker. A stale-source check makes the build fail if sources
changed without reimporting; it never silently replaces your Tiled work. The
optional `make_art.py --sources` recreates the original demonstration sources
and should only be used in a disposable copy. Normal builds preserve them.

The importer accepts a bounded finite orthogonal 16×16 profile with one shared
atlas and exact palette colors. Unsupported transforms/layers fail explicitly.
See the skill's Tiled reference before extending it.

## Checks and limits

```sh
python tools/check_levels.py
python tools/import_tiled.py res/world.json res/world.bin --check
```

The first needs a C17 compiler; the second needs only standard Python. Repository
tests add a wraparound cache oracle and original-68000 execution across regional,
debug and tour builds. Both maps and art are Main-resident in this Lab; larger
worlds should use the existing Megadev streaming/resource owners. No built disc,
BIOS or commercial artwork is included. Physical-console timing is not qualified
by host/model tests or an emulator review.

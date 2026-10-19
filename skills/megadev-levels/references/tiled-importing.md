# Tiled importing

The reference is Tiled's [JSON map format](https://doc.mapeditor.org/en/stable/reference/json-map-format/) and [global tile IDs](https://doc.mapeditor.org/en/stable/reference/global-tile-ids/). Inspect actual exported data instead of assuming field types or that a GID is a local tile index.

## Supplied profile

`import_tiled.py` accepts a `world.json` manifest (version 1) containing a palette path and one to four uniquely named areas with local `.tmj` paths. All inputs must remain beneath the manifest directory. Each area uses:

- Finite, orthogonal, right-down maps with 16×16 cells; 20–96 columns and 12–64 rows. At most 4,096 cells total across the resident world.
- One shared atlas tileset, embedded or external JSON `.tsj`, beginning at GID 1. It has 1–64 tiles, no margins/spacing/offsets, and an exact image/grid size. Tile image collections and Tiled tile animations need another profile.
- Numeric JSON tile layers named `Ground`, `Foreground` and `Collision`, all full-map size. Ground/Foreground are visible, opaque and unshifted. Collision may be hidden in Tiled; zero is walkable, any valid nonzero GID is solid.
- An `Objects` object layer with point objects of class `Spawn` and rectangular objects of class `Exit`. Names are unique within an area. Spawns have no properties; exits have string properties `target` (area ID) and `spawn` (destination spawn name).

Spawns and exits must fit the map and be walkable. Destination spawns cannot sit inside an exit trigger. Unknown object classes, transformed geometry, fractional coordinates, duplicate keys/IDs, missing targets and invalid candidate capacities fail explicitly.

Tile properties `palette` (2 or 3) and `priority` (boolean) define VDP attributes. `palettes.json` contains four lines of sixteen RGB triples, each component 0–7. The atlas PNG must use exact matching colors, with only fully opaque or fully transparent pixels. Opaque colors use indices 1–15. No resize, nearest-color substitution, alpha thresholding or dithering happens during import. Art preparation is a separate decision.

## GIDs and transforms

Tiled GID zero means empty. Clear all four high flag bits before finding a local ID, including the stale hex rotation flag. Horizontal and vertical flips are supported. The importer rejects diagonal transforms rather than applying the wrong VDP operation; pre-bake rotated artwork or extend the preparation pipeline.

A 16×16 tile becomes four 8×8 patterns in TL/TR/BL/BR order. Flipping swaps the quadrant selection and applies the same H/V pixel flip to each pattern. Changing only the VDP flip bits would leave the four quadrants in the wrong places.

## Prepare and verify

Install `scripts/requirements.txt` in a host virtual environment, then run:

```sh
python scripts/import_tiled.py /path/to/res/world.json /path/to/res/world.bin
python scripts/import_tiled.py /path/to/res/world.json /path/to/res/world.bin --check
```

The first command requires Pillow. It writes `world.bin`, `ground.bin`, `tileattrs.bin`, `palettes.bin` and `world.import.json`. The report binds source and output hashes. The second command needs only Python's standard library and runs inside the existing Docker image during normal builds. Editing a map or source image makes the build refuse stale prepared data until it is reimported. Output cannot replace source inputs.

The world bundle uses big-endian `WLD1` version 1 with a checked contiguous offset/length directory. Each `LVL1` area contains dimensions/counts, separate ground/foreground words, collision bytes, spawn records and linked exit records. The runtime checks lengths, ranges and destination indices before activating the world. This is an application format, not an SDK resource format. See `import_tiled.py` and `levels.c` for the executable encoder/decoder contract.

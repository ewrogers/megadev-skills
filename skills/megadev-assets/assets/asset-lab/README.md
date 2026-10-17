# Asset Lab

Two PixelLab-generated native 320×160 locations, a shared 16×24 balloon with two cels, and a small UI panel. The Lab loads location art from disc while retaining the shared sprite, UI and compatible palettes. Art style is illustrative; the pipeline is the lesson.

Generate a new project from the installed `megadev-assets` directory:

```sh
python3 scripts/create_project.py /path/to/asset-lab
```

From that new project:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue asset_lab.iso
```

PowerShell uses `source=$($PWD.Path)` in the mount argument. Linux users can optionally add `--user "$(id -u):$(id -g)"`. Use Linux containers. The template itself has metadata placeholders; generate a copy before building.

Load the CUE explicitly as Mega CD: `ares --system "Mega CD" /path/to/asset_lab.cue`. On macOS, use `open -a /Applications/ares.app --args --system "Mega CD" /path/to/asset_lab.cue`. Use a matching BIOS already supplied by the user; none is bundled.

- A: next location.
- B: toggle automatic changes every four seconds.
- C: reload the current location.

The first location loads automatically. Counters are decimal. Scene tiles reuse a 768-pattern VRAM region; the displayed count is not the whole VDP's free capacity. The Lab uses three palette lines, leaving one free.

Clean and build with `TOUR=1` for a four-load automatic QA tour, ending AUTO PASS after both locations reload. This tests operations, not controller mappings. Clean/rebuild with `TOUR=0` to restore the normal mode. For EU/PAL use `REGION=EU VIDEO=PAL`; Japan uses `REGION=JP VIDEO=NTSC`, with a matching BIOS and a clean rebuild. Keep any loaded disc immutable.

## Sources and preparation

`res/` includes original-sized PNGs, explicit palettes, prepared MDA1 data, reconstructed `.used.png` previews and hash reports. `res/reference/` records PixelLab requests, original outputs and generation provenance. Thirty generations were used. No resizing, opaque-pixel recoloring or palette reduction was needed; the two sprite cels were placed side by side without scaling.

PNG preparation uses Python 3.11+ and Pillow through the installed skill's `scripts/requirements.txt`. The normal Docker build needs no image library: it checks the prepared files, their source hashes and binary structure. Editing a PNG or palette requires explicit re-preparation, not just rebuilding.

For a background, call the installed `prepare_image.py` with PNG, palette and output paths plus `--backdrop --force`. The sprite uses `--frame 16 24 --force`; the panel needs only `--force`. Do not add snapping unless an intended source change needs it. `tools/make_panel.py` optionally regenerates only the simple shared-palette panel; it does not replace the PixelLab art.

MDA1 is a small teaching container, not a Megadev standard. The runtime validates CRC, section sizes and tile references, confirms Word RAM ownership, and uses the appropriate BIOS DMA variant. Bulk uploads run with rendering hidden. This is not a frame-budgeted streaming demonstration, a full renderer, or physical-console qualification.

Bootstrap foundation derives from this repository's Project Lab, with upstream portions retained under `LICENSE.megadev`.

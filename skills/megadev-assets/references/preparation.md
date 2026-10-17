# Prepare images and build

## Image preparation

The PNG helper needs Python 3.11+ and Pillow. Run it in an isolated host environment; the selected Megadev Docker image does not include Pillow. From the installed skill directory:

```sh
python3 -m venv /path/to/art-env
/path/to/art-env/bin/python -m pip install -r scripts/requirements.txt
/path/to/art-env/bin/python scripts/prepare_image.py /path/to/scene.png /path/to/scene.palette.json /path/to/scene.mda --backdrop
/path/to/art-env/bin/python scripts/prepare_image.py /path/to/actor.png /path/to/actor.palette.json /path/to/actor.mda --frame 16 24
```

On Windows use `python` and the environment's `Scripts/python.exe`. The palette is an explicit 16-entry JSON array. If opaque background art avoids index zero, omit `--backdrop`. `--snap` opts into channel-grid snapping; it never enables arbitrary palette reduction. `--force` deliberately regenerates existing outputs, with source-path checks still enforced.

Each conversion produces:

- `.mda`: prepared pattern/palette/map or frame data.
- `.used.png`: reconstructed native-size preview, including frame layout.
- `.asset.json`: input/output hashes, geometry, pattern use, CRC and conversion decisions.

The preview is reconstructed from the actual binary, not copied from the source. Check it at native size and a nearest-neighbor zoom. For a backdrop preview, verify that runtime will select the matching color. For sprite sheets, frame cells are read left-to-right, then top-to-bottom.

Keep source PNGs, palettes and prepared reports together in the project. `verify_prepared.py --root /path/to/project ...asset.json` rejects changed inputs/outputs, invalid binaries and paths escaping that project. It uses only Python's standard library, so the normal Docker build can validate checked-in prepared assets without an image-library dependency. It cannot prove a dishonest hand-edited report matches source pixels; regeneration tests provide that evidence.

## Asset Lab

Generate a disposable copy using the installed skill's `scripts/create_project.py`. It refuses existing destinations:

```sh
python3 scripts/create_project.py /path/to/asset-lab
```

The bundled PixelLab artwork is already prepared. From that generated directory:

```sh
docker pull ewrogers/megadev:latest
docker run --rm ewrogers/megadev:latest doctor
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue asset_lab.iso
```

PowerShell uses `source=$($PWD.Path)` in the mount argument. Linux users can add `--user "$(id -u):$(id -g)"`. Use Linux containers on Windows/macOS. To edit art, run the preparation helper again from its installed location, using `--force` after reviewing the new input; building alone intentionally refuses stale art instead of silently reconverting it.

US/NTSC is the default. Clean before changing region or optimization flags. For Europe build `REGION=EU VIDEO=PAL`; Japan uses `REGION=JP VIDEO=NTSC`, with a matching BIOS. `TOUR=1` enables automatic scene swaps; clean/rebuild with `TOUR=0` for the normal build. Public commands use `ewrogers/megadev:latest`; record actual tool/source revisions in evidence.

Load with `ares --system "Mega CD" /path/to/asset_lab.cue`, or on macOS `open -a /Applications/ares.app --args --system "Mega CD" /path/to/asset_lab.cue`. Use a BIOS already supplied by the user. Keep the loaded disc immutable; rebuild elsewhere and reload the full CUE.

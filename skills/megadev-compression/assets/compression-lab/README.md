# Compression Lab

Compare raw and Kosinski assets on Mega CD: file bytes, occupied disc sectors,
active buffer bytes, read time and decode/copy time. Artwork and an intentionally
random-noise case show both useful compression and a case that gets bigger.
The Lab is intentionally silent. The art, controls and MDA format illustrate
the technique, not a game template.

## Build

Generate a fresh folder with the skill's `scripts/create_project.py`, then:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue compression.iso
```

Open `compression.cue` as **Mega CD** in ares with your configured BIOS. On macOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/compression.cue"
```

Use your chosen generator ID if it differs. The CUE and ISO stay together. For
PAL, generate a fresh project and append `REGION=EU VIDEO=PAL` to `build`.
Choose `--platform linux/arm64` or `--platform linux/amd64` explicitly when
checking architectures. Use an absolute mount path on Windows; Unix users can
add `--user "$(id -u):$(id -g)"` for host-owned output.

## Try it

Left/Right selects **Compare**, **Load** or **Preload**. Up/Down selects Garden,
Cavern or Random Noise. B changes RAW/KOS. A runs; C cancels or drops a preload.
Start pauses the moving lantern, while loading service continues. Typical ares
keys are J/K/L for A/B/C and Enter for Start.

- **Compare:** A reads/decodes/uploads the chosen asset, then displays it. Try
  the same picture as RAW and KOS; it should look identical.
- **Load:** A fades out first, reads/decodes/checks/uploads, then fades in.
- **Preload:** A prepares the back VRAM slot while the old picture stays visible.
  When the status says **A SHOW PRELOAD**, A fades to it without another read.
  C drops it so a different selection can be prepared.

`FILE`, `DISC` and `RAM` describe the **selected** asset. `LAST READ` and `DEC`
are from the last returned job. READ is VBlank intervals, so 50/60 Hz must be
considered. Decode/copy uses the Sub stopwatch: **one T is 30.72 microseconds**.
`RANGE` means a timing interval could not be measured unambiguously. Validation,
Main/Sub handoffs and the bounded VRAM upload add time beyond these two fields.

RAM shows sector-rounded input plus decoded output bytes simultaneously in use.
The Lab reserves two 32 KiB Word RAM ranges (64 KiB total) regardless of that
number; it does not allocate a smaller pool automatically. RAW deliberately uses
this same staging/copy path for a fair byte-for-byte comparison. A direct raw
read into its final owned destination can avoid this extra input buffer.

Garden uses 8 RAW sectors or 4 KOS sectors; Cavern uses 11 or 5. Noise uses 8
RAW sectors or **9 compressed sectors**. The dots in Noise are intentional.
Both variants are included in this comparison disc; shipping a game normally
means selecting one representation, not storing both.

## Safety and evidence

The Lab validates every Kosinski source token, output bound and history
reference before calling Megadev's real `Kos_Decomp` assembly. Input/output stay
disjoint and Sub-owned. The decoded MDA header, palette, map and CRC are checked
before publication. A cancellation waits for any active read/decode to retire;
it never gives a still-written buffer to Main.

`build/compression-assets.json` records exact bytes, sectors, hashes and buffer
budgets. `TOUR=1` cycles all six sample/format pairs, consumes a preload, loads behind a
fade and checks completion. The
Main counter/lantern should keep moving during Sub work. Model tests execute
the actual decoder and adapters; they do not reproduce real seek timings.

The prepared garden/cavern/actor assets originate in the repository's
[Asset Lab](https://github.com/ewrogers/megadev-skills/tree/main/skills/megadev-assets/assets/asset-lab).
This folder includes the complete prepared files and validator. Bootstrap code
derives from Megadev; see `LICENSE.megadev`. The encoder, validator, renderer
integration and measurement policy are application code, not new SDK APIs.

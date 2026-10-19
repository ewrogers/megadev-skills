# Raster Labs

Two small standalone demos show moving water/reflection and two cameras sharing
a world. Native art, quiet PSG notes, colors, controls and view sizes are
illustrative. The effect uses the ordinary Mega Drive VDP; it does not depend on
extra Mega CD palette or sprite capacity.

## Build

From the installed skill, generate separate new folders:

```sh
python scripts/create_project.py water-game --demo water --id water
python scripts/create_project.py split-game --demo split --id split
```

In either generated folder:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue water.iso
```

Use `split.iso` or your chosen ID as appropriate. Keep the CUE beside the ISO.
For PAL, use a fresh project and append `REGION=EU VIDEO=PAL` to the build command.
Select `--platform linux/arm64` or `--platform linux/amd64` for architecture checks.
Use an absolute mount path on Windows. Unix hosts may add
`--user "$(id -u):$(id -g)"` for host-owned output. Make jobs must not exceed two.

On macOS, with your own configured BIOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/water.cue"
```

## Water controls

- Left/Right moves the orb/camera; Up/Down moves the waterline within safe bounds.
- A toggles the orb's underwater tint. B toggles reflected wave distortion.
- C enables/disables the automatic waterline sweep. Start pauses/resumes.

Reflection uses resident reversed tilemap rows, vertical tile flips and a
separate wet palette bank. HBlank switches Plane A's table and vertical scroll;
a fresh HBlank carries just two optional actor colors. The ball has its own
vertically flipped reflection with one coherent wave offset. VBlank restores the dry
state early. Foam masks the seam without relying on sprite overflow.

## Split controls

- D-pad pans the selected camera. A selects P1/P2.
- B toggles the split comparison. C automatically pans the other camera.
- Start pauses/resumes.

Both views share maps, patterns, palettes and a SAT. Per-line horizontal scroll
and a boundary VSRAM change produce two independent views. Actors are projected
into their own view; the divider occupies reserved sprite rows.

Typical ares bindings are J/K/L for A/B/C and Enter for Start. Check your actual
Mega CD bindings rather than assuming them.

## Counters and budgets

FPS normally reads 60 NTSC or 50 PAL. H counts consumed raster requests; L late
handler windows; T bounded-edge timeouts; S rejected VBlank setup intervals.
A fixed startup S increment is possible before the first settled frame; these
counters must not keep rising during normal operation. FPS alone does not prove
the reflection/split is rendering correctly.

The owner uploads up to 1,560 data bytes per accepted frame using bounded Main
Work RAM DMA for scroll, SAT and a contiguous four-row HUD. Top scroll and dry
palette restore happen before bulk work. Two 1,574-byte immutable snapshots use
3,148 bytes of RAM. HBlank does no C/BIOS/DMA work and every edge wait is bounded.
The two-color tint is optional; the reflected world uses prepared colors.

`TOUR=1` checks the model and runs a sustained sweep/pan. Tests cover reflection
coordinates, timing equivalence, scanline admission, vector/ABI preservation,
publication, pending/late interrupts, stuck edges and recovery. ARM64/AMD64 and
regional payload checks complement emulator auditions. Real hardware still
needs CRAM-dot, FIFO/bus timing, interrupt-latency and combined-workload checks.

`tools/make_art.py` creates original native pixel scenery and shared atlas;
`make_font.py` creates the readable font. No network/image service is needed.
Bootstrap code derives from Megadev; retain `LICENSE.megadev`. `rt_*` and the
raster handlers are application code, not SDK raster APIs.

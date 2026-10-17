# Graphics Lab — Lantern Quay

A small side-view harbor demonstrates Megadev rendering: two scrolling planes, parallax, animated/flipped airships, foreground occlusion, a fixed Window HUD and palette effects. It is a graphics example, not a game template or an art-style requirement.

## Build and try

Generate a new copy with the installed skill's `scripts/create_project.py`, then build from that directory:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue gfx_lab.iso
```

Use Linux containers on macOS/Windows. PowerShell uses `source=$($PWD.Path)` in the mount. Load `gfx_lab.cue` with `ares --system "Mega CD" /path/to/gfx_lab.cue`, using your own configured BIOS. Keep the loaded disc immutable while rebuilding elsewhere.

| Control | Action |
| --- | --- |
| D-pad | Move the controlled ship and scroll the camera. |
| A | Cycle Normal, Cycle, Fade and Flash palette modes. |
| B | Toggle the sky's slower parallax motion. |
| C | Put your ship behind/in front of the quay. Fly low near the pillars/railing to compare. |
| Start | Pause movement, animation and palette timing. |

With the usual ares mapping these are arrows, J, K, L and Enter. The emulator's Mega CD bindings are separate from its Mega Drive bindings.

The other ships show animation and horizontal flips. `SPR` counts admitted visible hardware sprites. `UPLOAD` counts data-port bytes for that frame, including the fixed palette, scroll and two HUD-row updates. It is not FPS or CPU load. No audio is included in this rendering example.

## Automatic and regional checks

Build a fresh copy with `TOUR=1` for an automatic camera/effects/priority tour. It stops after ten simulation seconds with AUTO PASS when all four effects were visited and bounded frames were submitted. This exercises runtime operations independently of synthetic controller input. Rebuild a clean copy with `TOUR=0` for normal controls.

US/NTSC is the default. For Europe use `REGION=EU VIDEO=PAL`; Japan uses `REGION=JP VIDEO=NTSC`, with a matching BIOS. Clean between configurations. The clock normalizes the example's movement and animation to 60 simulation ticks per second at either display rate; it does not measure or compensate for arbitrary missed frames.

## Art and integration

PixelLab generated two native 256×192 layers and two native 32×24 ship cels, using explicit CRAM-compatible palettes. The complete four-request run used 30 generations. Originals and exact prompts are in [res/reference](res/reference/README.md). Mirrored extension creates 512×192 repeating layers without resampling; sprite cels are combined into a 64×24 sheet. Preparation performs no visible recoloring. The sky's original dithering is retained; no extra dither pass was necessary.

Checked-in `.mda` files and their source/output hash reports build without Pillow. The tools verify that edited source art has been re-prepared; they never silently resize or reduce it. Source preparation belongs to the asset pipeline. `tools/make_font.py` generates the original opaque 5×7 glyph font inside 8×8 tiles using only Python's standard library.

`src/gfx.c` is the portable model; `src/ipx.c` owns the VDP and BIOS integration. Two frame buffers separate preparation from VBlank publication. Shared sprite patterns stay in VRAM; sky and quay load once through 2M Word RAM. The fourth palette is used, but the full per-frame upload cap is 24 hardware sprites, not the hardware maximum. See the installed skill's references for the allocation and source contracts.

Host tests and executed 68000/MMIO-capture checks are not display-timing simulations. Ares rendering does not qualify physical BIOS/drive variations, the Word RAM DMA quirk, maximum scanline behavior, FIFO timing or CD/audio coexistence. Upstream startup notices remain in `LICENSE.megadev`.

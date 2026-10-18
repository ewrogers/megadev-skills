# Streaming Lab

A small Megadev data disc: walk through a garden, preload a cavern while the
current art remains visible, then travel through a palette fade. Shared actor
and font patterns stay resident. The art and controls illustrate the technique;
they are not a required style or game structure.

## Build and try

From this folder, with Docker running:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue stream_lab.iso
```

The generated project's ID determines its ISO name. On a Unix host, add
`--user "$(id -u):$(id -g)"` if you need host-owned output. On Windows use an
absolute host path for the mount. Both Linux ARM64 and AMD64 images are supported.
For PAL, build a fresh project with `REGION=EU VIDEO=PAL`. `TOUR=1` enables the
automatic diagnostic tour. These are make assignments after `build`.

Open the CUE in ares as **Mega CD**, using your own configured BIOS. On macOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/stream_lab.cue"
```

- D-pad: move the resident actor. Moving right starts the next-area preload.
  The right edge travels when the next area is ready.
- A: travel to the other area; starts a load if its cache is empty.
- B: switch pattern uploads between 256 and 64 bytes per frame.
- C: discard the pending/next area. A live CD read drains before its result is
  discarded; press A or move left and approach the right edge again to reload. Current visible art stays valid.
- Up+C: deliberately request a missing file. The current area remains usable.
- Start: pause/resume movement. CD service continues; pending travel waits.

Typical keyboard mappings are J/K/L for A/B/C and Enter for Start; use your own
emulator mappings. HUD counters are hexadecimal. A missing file, cancellation
or drained timeout is reported as a discarded result, not a corrupt scene.
ERR should remain zero in normal manual use.

## What this proves

Sub's resident Megadev CD access loop reads whole files asynchronously. It
checks directory size before starting a sector-rounded transfer into a reserved
32 KiB region of 2M Word RAM. Main keeps using resident VRAM art while Sub owns
that RAM. After the matching response and ownership confirmation, Main validates
128 bytes per tick, copies bounded pattern chunks into its frame buffer, then
marks the inactive area ready after the final upload acknowledgement.

Two area pattern slots occupy VRAM tiles 192..831 and 832..1471. Shared actor
patterns use 128..139; font patterns use 32..127. The slots end at B800, before
the BIOS sprite table. Plane A (C000) holds HUD text; Plane B (E000) holds the
area map. There is no active Window plane. Palette 0 is UI, 1 is the actor, and
2/3 belong to the two areas. This is still the base VDP's four palette lines.

Main's linker reserves code at FF0000..FF8FFF, state at FF9000..FFCFFF, stack at
FFD000..FFDFFF and BIOS at FFF700+. Sub keeps its existing Program RAM layout.
The two 800-word cached maps, upload frames and validation metadata live in Main
state. This is a deliberately small residency example, not a general allocator.
No runtime code/stacks live in the Word RAM staging buffer. `ROOM.MMD` remains
a compiled startup-test fixture; the interactive demo does not execute it.

`stream_*`, MDA1, and `file_info.s` are original project helpers, not Megadev APIs.
The generator is self-contained. Bootstrap/layout/BIOS adapters derive from
Megadev's MIT starter; see `LICENSE.megadev`. Garden, cavern and actor art reuse
the original PixelLab assets prepared for this repository's Asset Lab; their
source, palette and generation records remain with
[megadev-assets](https://github.com/ewrogers/megadev-skills/tree/main/skills/megadev-assets/assets/asset-lab) in the source
repository. This copied Lab contains the complete prepared files and validates
their dimensions, checksum, palette and tile references before building.

No audio driver, compressed assets, direct Word RAM VDP DMA or 1M CDC pipeline
is included. Real drive latency, console/BIOS variants and audio/ASIC/DMA
coexistence still need physical checks. Ares is useful for functionality, not a
promise of hardware loading times.

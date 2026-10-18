# Transition Lab

A small journey through title, options, two areas, pause, results and credits.
It demonstrates game flow and safe scene cleanup around real CD loads. The art,
music, controls and collected-lantern goal are illustrative, not a game template
that the skill requires other projects to copy.

## Build and open

Generate this folder with the skill's `scripts/create_project.py`, then:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
```

The build creates **transition.cue**, the ISO data track and two WAV audio tracks
in `res/`. Open the **CUE**, keeping all referenced files together. Opening only
the ISO cannot play the external CD tracks. This Lab generates its own mixed-mode
CUE: do not replace it with the container's data-only `cue` command.

On macOS, with your own configured Mega CD BIOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/transition.cue"
```

A custom generator ID changes the ISO/CUE name. Use an absolute mount path on
Windows; on Unix, add `--user "$(id -u):$(id -g)"` for host-owned output when
needed. ARM64 and AMD64 Linux images are supported. Build a fresh project with
`REGION=EU VIDEO=PAL` after `build` for PAL. `TOUR=1` enables the diagnostic tour.

## Play

Use your configured Mega CD bindings. Typical ares keys are J/K/L for A/B/C,
Enter for Start and the arrow keys for the D-pad.

- Title: Up/Down selects Start, Options or Credits; A/Start opens it.
- Options: Left/Right selects gentle or brisk movement; B returns. This setting
  persists across scene changes. Credits returns with A/B/Start.
- Play: fly right toward the bobbing white-and-gold diamond. Once you reach it, A continues to the
  next area or the finish screen. The route is Quay, Grotto, then Results.
- B preloads the other area's art. C returns to the title, including while the
  preload is pending. Start pauses/resumes movement and actor animation.
- Results: A returns to the title; B begins another journey.
- Title C deliberately requests a missing area. It fades back to the same title
  with LOAD ERR increased by one. This is an intentional recovery check.

During travel, input is held until commit and fade-in complete. A delayed read
can hold the black view; a timeout never forces buffer reuse. If ownership cannot
be confirmed, WAITING remains visible and resources stay pinned. LOAD ERR counts
recoverable target failures; it is not a frame-rate or hardware-error counter.

Scene changes reset the local collected-lantern value only after success.
Returning from Options/Credits retains the title menu choice. Shared art and the
two cached area images remain resident. This is a fixed, bounded cache, not heap
allocation or an automatic unlimited scene loader.

## Lifecycle and resource boundaries

`scene.c` is a portable project state machine. `scene_service` in `ipx.c` composes
it with the reader, audio owner and VBlank publisher. Fade completion is checked
against the displayed palette. An old preload's generation lease is released
only after its reader/mailbox retires and Main owns 2M Word RAM. The next map
must be consumed before old local state is cleared and the generation advances.
A failed target leaves old progress and generation intact. One latest scene
request may queue; it cannot overwrite a live reader request.

Main/Sub service code stays resident. Pause never disables the Sub INT2 CD pump.
The existing Sub audio owner arbitrates CD music/data reads; music stops for a
read and restarts afterward. PCM samples remain resident. Area music selects one
of two original tracks; this is not a new driver or seamless CD crossfade.

Budgets are inherited explicitly from the standalone streaming/audio foundation:

- 32 KiB of 2M Word RAM for sector-rounded reads; Main owns it before validation.
- Two 640-tile VRAM slots ending at B800, before the BIOS sprite table. Font and
  actor patterns are shared below the slots. Both maps/palettes are retained.
- Validation checks at most 128 bytes per foreground update. Pattern uploads
  copy at most 256 bytes into a Main RAM frame; map replacement uses five
  320-byte VBlank payloads. No queued payload holds a Word RAM pointer.
- Changed palette lines publish first within the early-VBlank guard. UI text
  uses at most 24 changed cells per frame and stays opaque over both areas.
- PCM reserves four 4 KiB banks for the original bed and cues. Channel 1 is the
  bed; 2/3 are finite effects. Hardware has eight channels and 64 KiB.
- Scene-local state is a reused fixed block with generation-checked leases.
  Retained cache storage is counted separately; leaving a scene does not
  misleadingly claim to free those retained bytes.

## Provenance and validation

The `scene_*`, `stream_*` and `audio_*` helpers, MDA1 format and C ABI adapters
are project code, not Megadev APIs. Bootstrap/layout files retain Megadev's MIT
notice in `LICENSE.megadev`. The complete prepared garden/cavern/actor files
come from our original PixelLab Asset Lab art, with source/provenance in
[megadev-assets](https://github.com/ewrogers/megadev-skills/tree/main/skills/megadev-assets/assets/asset-lab).
The audio synthesis tool generates our original music and cues locally. No
commercial media or BIOS is included. ROOM.MMD is only a startup/ABI fixture;
scene behavior stays in resident IPX.

The automatic tour (`TOUR=1`) exercises menus, both areas, results, a cancelled
preload, re-entry and a missing-area rollback. It ends with AUTO PASS if cleanup
and recovery counters agree. It does not prove audible quality or physical seek
timing. Run the normal build for input/listening checks, and repeat with PAL.

The repository also executes actual compiled 68000 policy/coordinator code,
paired reader and audio adapters against models on ARM64/AMD64. Real consoles,
BIOS variants, drive retries, stack maxima and electrical timing remain hardware
checks. Dynamic module unloading, 1M streaming and ASIC coexistence are outside
this example.

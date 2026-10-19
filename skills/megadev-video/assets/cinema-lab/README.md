# Cinema Lab — Lantern Flight

A small original airship animation exercises streamed frame groups, read-ahead,
complete-page presentation, resident PCM and safe skip/pause. Art, sound and
controls are illustrative. It is a native tile animation player, not a general
photographic FMV encoder. Target: 160×96, 16 palette entries, nominal 10 images/sec.
The console still refreshes at 60 Hz NTSC / 50 Hz PAL; that is a different measure.

## Build

From the installed skill:

```sh
python scripts/create_project.py cinema-game
```

In the generated folder:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue cinema.iso
```

Use your ID if different. For PAL, generate a new folder and append
`REGION=EU VIDEO=PAL` to `build`. Add `--platform linux/arm64` or `linux/amd64`
as needed; Unix hosts can add `--user "$(id -u):$(id -g)"`. Use an absolute host
mount path on Windows. No Python graphics package is needed: the standard-library
exporter creates original native pixels, packed tiles and sign-magnitude PCM.

With your own configured BIOS on macOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/cinema.cue"
```

## Try it

- A plays/replays the 48-image sequence.
- B switches 4,096-byte / deliberately slow 256-byte upload batches.
- C skips and drains outstanding data access before allowing replay.
- Start pauses/resumes. A committed packet can retire before the hold takes effect.
- Typical ares keyboard mappings: J/K/L = A/B/C, Enter = Start. Verify your profile.

IMAGES/SEC counts actual presented images. LATE counts presentations later than
the nominal deadline; slow mode should increase it while preserving complete
images. The previous image remains visible while buffering. After frame 48,
THE END permits replay. The short PCM score is resident, so it does not contend
with video data reads; periodic cues follow presented-frame events.

## Bounds and tests

One Main frame (7,680 bytes), one copied DMA packet (at most 4,096 bytes), two
Sub PRG groups (126,976 bytes) and two VRAM image slots (15,360 bytes) are reserved.
Code, font/poster/audio constants, stacks, HUD and state are additional costs.
Each disc file holds eight frames and occupies exactly 31 data sectors. The full
movie payload is 380,928 bytes. Header checks and sums reject accidental damage;
sum16 is not a cryptographic integrity mechanism.

`python tools/check_cinema.py` runs portable C17 O0/O3 timeline/packet tests with
a host compiler. The repository adds independent pixel/format checks, actual
original-68000 Main/Sub execution and native ARM64/AMD64 regional build comparison.
TOUR=1 starts automatically for emulator demonstrations. Source changes to the
exporter regenerate frames during `build`; generated discs are not included.

No active-display palette writes, CD-DA, general video decode or long streamed
voice are implemented. Real drive latency, VDP/DMA timing, PCM quality and
physical-console behavior need separate qualification. Preserve `LICENSE` and
`LICENSE.megadev` when adapting the original example.

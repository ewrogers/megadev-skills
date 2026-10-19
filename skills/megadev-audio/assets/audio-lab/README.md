# Audio Lab

A standalone Megadev sound desk for CD music, Sega CD PCM effects and deliberate
music/data handoffs, streamed voice and longer samples. Arrangements and sounds are illustrative;
the skill does not prescribe a musical genre, art style or control scheme.

## Build and open

Generate this folder with the skill's `scripts/create_project.py`, then:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
```

The build creates **audio_lab.cue**, the ISO data track and two WAV audio tracks
in `res/`. Open the **CUE**, keeping all referenced files together. Opening only
the ISO cannot play the external CD tracks. This Lab generates its own mixed-mode
CUE: do not replace it with the container's data-only `cue` command.

On macOS, with your own configured Mega CD BIOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/audio_lab.cue"
```

A custom generator ID changes the ISO/CUE name. Use an absolute mount path on
Windows; on Unix, add `--user "$(id -u):$(id -g)"` for host-owned output when
needed. ARM64 and AMD64 Linux images are supported. Build a fresh project with
`REGION=EU VIDEO=PAL` after `build` for PAL. `TOUR=1` enables the diagnostic tour.

## Controls

Left/Right changes pages. Up/Down changes the selected track/effect.
Start requests silence for CD music and PCM voices. Typical ares keys are
J/K/L for A/B/C and Enter for Start; use your configured mappings.

| Page | A | B | C |
| --- | --- | --- | --- |
| CD MUSIC | Play selected track on repeat | Pause/resume | Stop CD music |
| PCM DESK | Audition selected effect | Cycle center/right/left pan | Toggle looping PCM bed |
| DRIVE TEST | Load the other area | Request CD track 2 | Toggle PCM bed |
| VOICE STREAM | Play selected long clip | Play a resident bell | Stop voice; drain a pending read |

The first CD track starts once the initial area is ready. Try both tracks,
then PCM's bell, pluck and drum. Holding A does not repeatedly fire a cue.

On VOICE STREAM, Up chooses speech or a 14-second chime sample. Both are larger
than the PCM chip's 64 KiB RAM. A buffers the first two chunks, then plays while
the actor/counter keep moving; B still plays an effect. CD music pauses for
streaming and restarts from the beginning afterward. C stops the voice promptly
but retains an active disc read until it retires. Changing pages does not stop
audio. A new voice start is refused while a voice or scenery read is busy.

Down selects a deliberately slow upload cap for the **next** start. Initial
buffering takes several seconds; eventually the next chunk is late and the
voice waits in silence, then rebuffers. This is an underrun demonstration, not
normal playback. Stop, turn the slow test off and play again for normal sound.

On DRIVE TEST, turn the PCM bed on and play CD music before loading. CD music
intentionally stops for the data read and restarts from the beginning afterward.
The resident PCM bed keeps playing; you can switch to PCM DESK and audition
sounds during the read. Start silence updates desired music state, so the read
must not restart stale music afterward. Scenery switches through a palette fade
once its bounded validation/upload finishes.

PCM BED/FX1/FX2 are manager activity indicators, not measured audio levels.
The Main counter is hexadecimal and should keep advancing. ERR should remain
0000. A 30-second failed command/ack deadline quarantines that path; it does not
pretend to stop a stuck hardware operation. Reset the disc after a diagnostic
fault rather than clearing its state underneath an operation.

## Contents and budgets

- `make_audio.py`: original music synthesis plus a small sign/magnitude encoder.
  CD tracks are stereo 16-bit 44.1 kHz, sector-aligned, with actual two-second
  silent pregaps. PCM payloads exclude FF loop markers and have explicit tails.
- PCM RAM: four 4 KiB banks (16 KiB) reserved for the looping bed and three cues.
  Hardware channel 1 is the bed, 2/3 are prioritized finite effects. The hardware
  has eight channels and 64 KiB; this is an example policy, not an engine limit.
- Long samples use channel 4, two 16 KiB PCM halves at `4000–BFFF` and one
  16 KiB Sub Program RAM staging buffer. The source is **not** loaded in full.
  `make_voice.py` creates sector-aligned files, marker/checksum tables and
  `build/voice-assets.json` with sample sizes, hashes and buffer budgets.
  See [voice provenance](res/VOICE.md) for the synthesized speech.
- Main Work RAM keeps code, UI/graphics frames and two area maps. Sub Program RAM
  keeps the PCM samples, manager and BIOS/reader adapters. No code/stack is
  executed from the 2M Word RAM staging buffer while it changes ownership.
- Rendering reuses the Streaming Lab's two bounded VRAM slots, shared actor/font
  and early-VBlank palette guard. Only changed palette lines are written; the
  map switches only after the displayed area palette is black.
- Audio uses COMCMD4..6/COMSTAT7, independently of the reader's command channel.
  One PCM/BIOS owner runs in Sub foreground; INT2 continues the CD access pump.

`audio_*`, `stream_*`, `voice_*`, MDA1 and the C ABI shims are original project helpers, not
Megadev APIs. Bootstrap/layout code derives from Megadev's MIT starter; see
`LICENSE.megadev`. The prepared garden/cavern/actor files are the repository's
original PixelLab Asset Lab art; their source/provenance is kept with
[megadev-assets](https://github.com/ewrogers/megadev-skills/tree/main/skills/megadev-assets/assets/asset-lab).
This copied project includes the complete prepared files and their validator.
`ROOM.MMD` is retained only as a linked startup-test fixture.

No FM/PSG driver, compressed sound, arbitrary live-bank replacement or 1M CDC
pipeline is implemented. Streaming uses bounded whole-file CDC reads, not a
new continuous-sector SDK API. Keep the Sub worker serviced; an arbitrary CPU
stall can exhaust/repeat an armed half before software runs again.
Real drive/BIOS variants, audible quality, electrical timing and future
FM/Z80/ASIC/DMA coexistence still need their own checks.

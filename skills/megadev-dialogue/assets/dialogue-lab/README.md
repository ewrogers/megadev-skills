# Dialogue Lab: The Glass Conservatory

Two short conversations demonstrate portraits, typed text, punctuation pauses,
styles, paging, branching choices and paired panels. Two greeting excerpts use
finite resident PCM voice. The original art, story, voices and controls are
illustrative; they are not a style or design requirement for your game.

## Build

Create a new folder using the skill's `scripts/create_project.py`, then run there:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue dialogue.iso
ares --system "Mega CD" dialogue.cue
```

PowerShell uses `${PWD}` and CMD `%CD%` in the mount string. Docker runs Linux
ARM64 or AMD64. Ares needs your own configured regional BIOS; none is included.
Press Start at the BIOS screen if needed. A clean `REGION=EU VIDEO=PAL` build
uses a compatible European BIOS; US/JP NTSC are also tested.

## Controls

- D-pad selects stories or responses. A opens, reveals, then advances on a later
  press. Holding A never accepts multiple lines.
- B returns to stories, including during voice. C toggles voice excerpts.
- Start pauses reveal and stops the current excerpt. Resume continues subtitles.
- Voice Settings changes the same option; subtitles always remain enabled.

Use your emulator's mappings: J/K/L mean A/B/C only when configured that way.
Try both Clockwork Bloom responses. The wind branch has a second page and a
styled “Listen.” Notes Across the Room retains the previous speaker above the
current one. First excerpts say “Welcome, traveler” and “Listen to the leaves.”

Pages present atomically after bounded row uploads. A brief page-change delay is
intentional. The mouth poses follow text reveal, not phoneme-level lip sync.
The example uses UI cues and voice, with no background music or runtime voice
streaming. Returning during a committed start safely retires it before stopping.

## Checks and authoring

```sh
python tools/check_dialogue.py
python tools/make_assets.py
python tools/make_audio.py
python tools/preview.py preview
```

Executable checks/preview need a host C compiler. Python conversion/PNG encoding
uses the standard library. The preview folder must be new. WAV sources are
committed, so building requires no speech service or synthesizer. Optional
`python tools/make_voices.py new-voice-folder` uses eSpeak NG; inspect new WAVs
and copy them deliberately rather than overwriting source audio automatically.
The PCM encoder rejects oversized or stereo sources.

`TOUR=1` walks both conversations through the real model/presentation path.
Repository checks execute original-68000 code and paired PCM adapters, build both
architectures and compare payloads. These are not real VDP/PCM/timing simulation.
The original example is MIT; Megadev startup notices remain in `LICENSE.megadev`.

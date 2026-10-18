# Test Lab

A small replay course and a separate live CD/audio integration suite. Collect
three gold lights and fly through the right-hand gate; record a route, then check
that replay reproduces its logical state. A deliberate movement bug demonstrates
first-divergence reporting. The course's gate-ready event is a controlled fixture,
not a live CD completion. The second page exercises real emulated disc/audio work.
Art, music, rules and controls illustrate testing and do not prescribe a game style.

## Build and open

From the installed skill folder:

```sh
python3 scripts/create_project.py my_test
cd my_test
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
ares --system "Mega CD" test_lab.cue
```

The build makes a mixed-mode CUE with two original music tracks. Keep its ISO and
`res/track02.wav` / `track03.wav` together. Supply your own BIOS. Docker uses Linux
containers on ARM64/AMD64; PowerShell uses `${PWD}` in mount arguments. For PAL,
use a fresh generated copy with `build REGION=EU VIDEO=PAL`. `build TOUR=1` enables
the automatic replay/bug/recovery/service tour.

## Replay course

| Pad | Action |
| --- | --- |
| D-pad | Fly; collect the three gold lights, avoid the red marker |
| A | Start a new recording, or stop recording |
| B | Replay the saved tape from its initial snapshot |
| C | Toggle the deliberate movement-speed bug |
| Start | Live CD/audio page; suspends course simulation |

Record with BUG OFF, move for a few seconds, then stop with A and replay with B.
MATCH means every recorded tick matched. Toggle BUG ON and replay a moving tape:
DIVERGED preserves the first bad zero-based tick. Turn it off and replay again.
Live D-pad input is ignored during playback. FULL stops recording at 720 ticks
(12 seconds); B still replays it. A new recording replaces the in-memory tape.
Counters are hexadecimal, and `----` means no divergence. No recordings are saved
to backup RAM or disk. The green gate opens after three logical seconds; collecting
all three lights permits a goal at the right edge and resets them for another lap.

## Live CD + audio

Start switches pages. A runs the suite, B selects normal/64-byte uploads, C cancels
through the existing safe service path. The suite plays music, loads the other
background and resumes music, requests the second track, tests a missing file,
then loads valid content again. PASS counts three successful checks per completed
run. A can start another run; a service error/deadline produces SUITE FAILED.
Changing pages keeps the services running. Loads may interrupt CD music because
the same drive is used for data; the selected track resumes afterward.

## Automated checks

```sh
python3 tools/verify_replay.py
python3 tools/verify_replay.py --inject-bug
```

A host C17 compiler is required. The first prints passing JSON; the intentional
bug command exits 1 with divergence at tick 0. Equal-time checks compare 240 NTSC
with 200 PAL VBlanks. This tool tests portable logic; the repository additionally
executes the compiled original-68000 replay and Main/Sub adapters.

Runtime VDP writes use the existing bounded VBlank publisher. Palette updates
are restricted to the early blank window. Shared art stays resident while Word
RAM changes owners. A fatal/stalled peer does not authorize buffer reuse.
The unused Room module is a startup fixture, not another active service loop.
Real console/drive/BIOS timing and audible quality require separate qualification.

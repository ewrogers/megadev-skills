# Co-op Lab — The Lantern Keepers

Light eight gold lanterns in a small garden. Up to four keepers can join while
others play. Moths cause damage; A casts a visible pulse that stuns nearby moths.
Each keeper has three HP and returns independently after three seconds when down.
The setting, art, sounds, controls and tuning illustrate the mechanics, not a
required design for other games. All art is original and drawn at native pixels.

## Build

Generate into a new directory (existing projects are refused):

```sh
python skills/megadev-multiplayer/scripts/create_project.py "build/co op" --id coop
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD/build/co op,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD/build/co op,target=/workspace" ewrogers/megadev:latest cue coop.iso
```

Use `REGION=EU VIDEO=PAL` for Europe. `EA=1` explicitly selects EA 4 Way Play;
AUTO supports normal pads and Team Player. Build `TOUR=1` to boot directly into the four-player autoplay (normal builds also offer it from the title). Use a clean generated project when changing build-time video/adapter/tour options. The reader is application code from
Input Lab, not an SDK multitap API. At most four of eight detected socket
positions can claim a keeper. Mice do not join this pad-controlled demonstration.
Power/controller-adapter behavior still needs device testing on real hardware.

On macOS launch explicitly with:

```sh
open -a ares --args --system "Mega CD" "/absolute/path/co op/coop.cue"
```

With an already-running ares instance, verify its loaded path actually changed;
some launchers only focus it. If needed quit that instance normally and relaunch,
or use its Load menu as a fallback. Avoid creating multiple instances and then
capturing a different one. The BIOS Start binding can differ from other systems.
Do not distribute BIOS files with the project.

## Controls

- Each pad: Start joins. Once joined, Start pauses/resumes globally.
- D-pad walks; A stuns, with a white three-second recharge bar.
- Hold Start+B for one second to leave. Other keepers keep their player numbers.
- C at the title starts a labeled four-CPU tour. It routes around obstacles, lights all eight lanterns, celebrates for four seconds and repeats. C or Start leaves that tour.
- A after clearing restarts the garden. A fanfare and animated stars mark clear.

In our configured ares profile J/K/L map to A/B/C. Use your own mapped Start.
A second configured controller joins from socket 2A; Team Player's positions are
1A–1D and 2A–2D. Player HP colors match cloaks; recharge bars stay white.
A disconnected claim waits five seconds of service time; the pawn stops and is
excluded from the camera. Same-kind reconnect retains HP and recovery state.

The camera frames connected living keepers. Movement stops at the party's
separation bound; regroup to continue. Players can overlap, so returning near a
teammate cannot block a narrow passage. Returning keepers briefly flash with
immunity. Pause freezes gameplay, but connection/leave service continues.

## Validation

`python tools/check_coop.py` runs O0/O3 host behavior checks with undefined-behavior
sanitization on macOS/Linux. Repository tests add randomized rosters, asset regeneration,
compiled original-68000 protocol/IRQ/model/publication and fanfare tests, and
fresh US/JP/EU/debug/tour/EA container builds on ARM64/AMD64. Host models and
scripted input do not prove peripheral timing or actual CPU headroom.

The Lab holds all graphics resident and uses Main for authoritative gameplay.
Sub only supports boot and normal INT2 service; the unused ROOM module is the
standard startup regression fixture. It does not split players across CPUs,
stream room data, or add new Word RAM handoffs during play. Integrate those
through scenes/system/streaming/memory when a real game needs them.

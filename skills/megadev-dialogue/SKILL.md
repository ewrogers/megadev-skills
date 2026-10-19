---
name: megadev-dialogue
description: Add text boxes, portrait conversations, timed text, choices and branching to Megadev Sega CD/Mega CD games. Use for dialogue presentation and optional PCM voice coordination; keep input drivers, general menus, scene lifetimes, localization and CD/audio services with their existing owners.
---

# Megadev dialogue

Adapt the conversation to the user's game. Dialogue Lab's conservatory, portraits,
voice excerpts, controls and branching story are illustrative, not a prescribed
setting, aesthetic or conversation structure. Megadev supplies hardware and BIOS
interfaces; it does not supply this example's dialogue engine or SGDK APIs.

## Separate script, state and presentation

Read [scripts and reveal](references/scripts-and-reveal.md) for bounded text spans,
wrapping, paging, inline styles, delays, branches and reveal/advance semantics.
Declare glyph coverage and the maximum panel size before importing text. Validate
content and destination IDs rather than treating arbitrary text as executable code.

Reuse the game's input snapshot and clock. Accept, cancel and pause use edges;
choice navigation can use the UI owner's bounded repeat. Remember focus, keep
subtitles readable without voice and require a distinct action to advance after
instant reveal. Do not inherit the Lab's exact timing or choice behavior blindly.

Read [presentation](references/presentation.md) for panel bounds, portraits,
paired speakers and source lifetimes. Reuse the renderer's single publication
owner and palette budget. Begin reveal/voice only after the correct page is
acknowledged; a stale render completion must not start another line's voice.

## Treat voice as optional audio work

Read [voice and ownership](references/voice-and-ownership.md) when adding speech.
A conversation requests playback through the existing audio owner. It must not
install another PCM driver, take the drive independently, or silence unrelated
channels just to stop a line. Keep subtitles usable if voice is disabled, missing,
busy or unavailable.

Use short resident PCM, streamed PCM or CD audio according to the game's memory,
seek and channel budgets. Dialogue Lab preloads two short PCM excerpts; it does
not demonstrate runtime disc voice streaming. Skips and cancellation retire a
pending start, stop any stale playback, then allow the newest line. A timeout
quarantines the audio request rather than making its resources reusable.

## Validate the conversation

[Dialogue Lab](assets/dialogue-lab/README.md) is a standalone two-speaker example
with branching choices, styled text, paging, paired panels and finite PCM voice.
Use `scripts/create_project.py` to copy it to a new folder. Original art, committed
voice sources, deterministic encoders, host checks and native previews are included;
installing another skill or speech synthesizer is not required to build it.

Read [testing and actual APIs](references/testing-and-apis.md) for source links,
O0/O3 contracts, original-68000 execution, paired Main/Sub PCM checks and the
ARM64/AMD64 PAL/NTSC matrix. Keep CPU models, emulator observations and physical
console evidence distinct. Pull `ewrogers/megadev:latest`; use two Docker CPUs.

UI owns presentation primitives/navigation policy, graphics owns VDP submission,
audio owns voices/drive access, streaming owns file/preload lifetimes and scenes
owns conversation entry/exit resources. Dialogue adds script progression and
coordinates those services; it does not replace them.

---
name: megadev-video
description: Build streamed Sega CD cutscenes with Megadev using prepared frame data, read-ahead buffers, bounded VDP uploads, audio coordination and safe skip/recovery. Use for timed image sequences or FMV pipeline planning, not ordinary sprite animation alone.
---

# Megadev video

Start from the project's cutscene intent, source footage/art, desired resolution,
image rate and audio policy. Cinema Lab's airship, palette, music and controls are
illustrative. Do not copy its story or presentation into another game by default.

Megadev supplies CD/VDP/PCM facilities, not the CIN1 player or a general movie
codec. Inspect the selected `ewrogers/megadev:latest` source before using APIs.
Do not import SGDK drivers or assume a desktop video format plays on the console.

- Read [Pipeline and budgets](references/pipeline.md) for native frames, changed
  tiles, photographic footage, disc rates, RAM/VRAM and frame-rate decisions.
- Read [Playback and ownership](references/playback.md) for Main/Sub buffers,
  immutable uploads, presentation clocks, audio and draining a skipped read.
- Read [Testing](references/testing.md) for format failures, executable adapters,
  regional/architecture matrices and evidence that still needs real hardware.

Essential contracts:

- Preprocess assets into a validated target format. Check dimensions, tile and
  palette indices, decoded lengths, sector-rounded writes and maximum work per
  frame. Keep authored captions/subtitles separate from packed image data.
- Budget CD reads, CPU decoding, copies and VDP uploads independently. A 60 Hz
  display does not imply 60 new video images/sec. Measure actual presented
  images and stalls rather than claiming a rate from a timer constant.
- Read ahead into fixed buffers. Only reuse a buffer after its producer and
  consumers have retired it. Preserve the last complete visible image if late.
- Publish copied, bounded Main RAM upload batches through the existing VBlank
  owner. Never flip to partial frames or leave stale Word RAM pointers queued.
  Reserve every pattern slot, name table, SAT, scroll and Window range explicitly.
- Pick one synchronization policy. Use frame-presentation events for illustrative
  cues; sustained synchronized voice/music needs an audio-clock and buffering
  design, not simply starting two unrelated timers together.
- Serialize drive use. Concurrent CD-DA music and video data reads are not an
  independent dual-drive workflow. Resident/streamed PCM has its own RAM, refill
  deadlines and ownership requirements.
- Skip, pause, errors and timeouts must preserve ownership. A timeout is not proof
  the reader stopped. Drain outstanding CDC access and replies before scene
  teardown, resource reuse or starting another drive operation.

[Cinema Lab](assets/cinema-lab/README.md) includes a self-contained procedural
native-art exporter and a full-frame 4bpp player. It is not a photographic FMV
encoder, compression library or sample-accurate long-voice player. Its `ci_*`
and `cinema_*` helpers are application code, not Megadev APIs.

Reuse `megadev-streaming`/`megadev-system` for shared loading/handshake policy,
`megadev-audio` for PCM/CD ownership, `megadev-scenes` for transitions,
`megadev-localization` for captions and `megadev-compression` for bounded decode.
Video adds frame packaging, presentation and synchronization contracts without
installing competing drive, VBlank, audio, input or save services.

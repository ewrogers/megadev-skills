# Streamed PCM voice and long samples

Use resident samples when they fit. Stream when the clip's length justifies
drive ownership, extra buffering and recovery logic. This extends the same
audio owner; it is not a second driver or a required dialogue architecture.

## What Megadev actually exposes

Audited SDK: `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` in
`ewrogers/megadev:latest`. Recheck the selected image's sources after updating.

- [`cdrom.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/cdrom.h)
  exposes `filename`, `filebuff`, `filesize`, `access_op` and
  `access_op_result`. The Lab commits `CDROM_LOAD_CDC` last, with interrupts
  briefly masked, then keeps servicing the foreground while INT2 advances
  the SDK's existing access loop. Its blocking `load_file()` wrapper is not
  used for runtime voice refills.
- [`cdrom.s`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/cdrom.s)
  loads whole files, copies whole 2048-byte sectors and caches packed 22-byte
  directory records. The Lab's `stream_file_size` is an application C-ABI shim
  around `find_file`, not the SDK's unfinished C convenience wrapper.
- [`pcm.def.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.def.h)
  exposes `_PCM_PLAY_CH4_L/H` for the fourth channel's current sample address.
  [`pcm.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.h)
  provides channel/wave-bank selection and the start, loop and pitch registers.
  There is no audited high-level streaming voice API. `voice_*` is Lab code.

Do not invent a file-offset or bounded-sector SDK call. The example splits
each long sample into sector-aligned files so the public whole-file reader has
a bounded destination. A custom continuous-sector reader is a different design
that needs its own BIOS, scheduling and cancellation audit.

## Example buffers and bandwidth

| Resource | Example reservation |
| --- | --- |
| Resident PCM bed/effects | `0000–3FFF`, unchanged during streaming |
| One-shot silence loop | `3F00`: zero, then FF marker back to zero |
| Streamed voice, hardware channel 4 | half 0 `4000–7FFF`, half 1 `8000–BFFF` |
| Other PCM RAM | `C000–FFFF` unused by this streamer |
| Sub Program RAM | One 16 KiB sector-read staging buffer; small state/checksum tables |
| Sub module data/BSS reservation | `24000–2BFFF`, including staging/state and unused headroom |
| Sub stack | Existing `1E000–1FFFF`, outside those reservations |
| Word RAM | Not used by voice refills; existing scenery loader retains its ownership protocol |

Each 16384-byte chunk contains 16383 sample bytes and an FF marker. The last
chunk is padded with zero samples, so completion can include up to about two
seconds of silent tail. Chunk files are validated at authoring time; the runtime
checks directory length **before** permitting a sector write, completion result,
reported size, marker placement and a generated 16-bit additive checksum before
publishing a half as ready. The checksum detects common accidental damage; it
is not authentication or a collision-resistant integrity format.

The US release links 16524 bytes of Sub BSS plus 20 initialized bytes, rather
than using all 32 KiB of that reservation. Resident sample constants stay in
the Sub module's code/constant range. Recheck linked sizes when changing clips,
state or compiler options; do not count a reservation as actual live allocation.

At 8 kHz mono, a half lasts about 2.05 seconds. The normal upload cap is 512
bytes per distinct VBlank: 25 KiB/s at PAL, 30 KiB/s at NTSC, before CPU and
drive costs. Filling a half takes 32 service frames; the remaining time must
cover seeking, reading and scheduling. The 64-byte diagnostic cap deliberately
cannot keep up. These are configured byte caps, not measured hardware guarantees.
Choose rates, buffer sizes and layout from the actual game's worst case.

## Playback and retirement

1. Reserve the existing drive owner; stop CD-DA and wait for its BIOS command
   to retire. Queue later music requests as intent. They must not seek during
   a data read. The sample files belong to the ISO, not CD-DA audio tracks.
2. Read and upload two halves before enabling the voice. Never overwrite a
   playing or ready half. Keep the marker's loop target on silence until the
   next **complete, validated** half can be armed.
3. Poll the sample address with a bounded high/low/high read. A changed high
   byte retries; repeated tearing or an impossible address stops safely.
   Consume a half only when the observed playhead leaves it, not when the
   nominal number of PAL/NTSC frames has elapsed.
4. When the next half is late, the marker lands in silence. Count the underrun,
   refill the next unplayed chunks, then resume. Partial chunks and previously
   consumed chunks must not be presented as new audio.
5. Stop disables only the voice channel immediately. An outstanding CDC read
   still owns its staging destination and drive until it really retires.
   Reject replacement starts during draining. A deadline reports a fault; it
   does not free the buffer or issue another drive operation underneath it.
6. At finite completion, disable the channel and release the drive. The latest
   requested CD music resumes from its beginning; stale music is not restored
   after a silence request. Resident effects and the bed use their original
   channel-mask owner throughout.

Service the worker regularly. The example stops before refilling after an
observed gap greater than one second. Two buffers and a mutable loop address
cannot promise correct playback through an arbitrary CPU stall: an already
armed loop may repeat before the worker runs again. Do not advertise unlimited
stall tolerance, hard-real-time guarantees or uninterrupted CD-DA plus data
seeking. Real drives need cold/warm seek and concurrent-workload qualification.

The scenery reader defers new requests while voice owns the drive. Voice starts
are refused while a scenery request is already pending. This simple exclusion
policy avoids deadlock; a game's central scheduler may prioritize or queue them
differently. Reuse its mailbox, drive and PCM owners rather than copying a rival.

## Authoring and quality

`tools/make_voice.py` accepts a mono PCM16 WAV at 8 kHz, converts it to RF5C164
sign/magnitude (no FF in sample data), and generates chunk files, checksum tables
and a JSON budget/hash manifest. It also authors a 14-second chime sample.
The committed speech is an original synthesized reading; its provenance is in
`assets/audio-lab/res/VOICE.md`. No speech engine or FFmpeg is needed to build.

Resample with an anti-alias filter **before** importing a different rate; dropping
samples or linear interpolation alone is not a quality downsampler. Preserve
headroom and audition seams, sibilants, silence and overlap with effects. A
higher rate improves bandwidth but consumes the refill margin faster. Pitch
comes from the chip's fixed clock and step register, not PAL/NTSC frame counts.
The [ares PCM implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/pcm.cpp)
corroborates this model's marker, fractional-address and playhead-read semantics;
it is emulator evidence, not a substitute for listening on a console.

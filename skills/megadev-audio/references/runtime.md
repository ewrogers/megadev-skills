# Runtime ownership and sound policy

Keep sound requests separate from hardware register access. A request identifies
an effect/track, desired pan/priority and optional lifecycle owner. The sound
manager chooses channels and enforces memory/drive contracts. Sound style,
polyphony and stealing rules belong to the game, not this example.

## Resident PCM

Audio Lab reserves four 4 KiB banks (16 KiB total). It copies samples only at
startup with all channels disabled, using the Sub-visible odd byte lane. Bed,
bell, pluck and drum have separate addresses; a zero/marker tail at 3F00 is
reserved for one-shot silence. The optional [streaming path](streamed-pcm.md)
reserves two additional 16 KiB halves; it never replaces these resident banks.

Hardware channel 1 is the looping bed. Channels 2/3 serve effects; channel 4
serves the optional long-sample streamer. Channels 5..8 remain unused. This is
an illustrative split, not a four-channel hardware limit.
The model selects a free effect slot, otherwise the lower-priority slot;
a lower-priority request cannot steal it. Equal priorities may replace that
slot. Effects have finite release times in a 300 Hz clock shared by PAL/NTSC.
The source sample already fades; expiration explicitly disables the hardware
channel. Bed lifetime is independent. Channel settings preserve the other
enable bits, and pan uses independent left/right four-bit gains.

The Sub foreground loop is the only PCM selector/register owner. It uses the
same four-NOP settings settle interval illustrated in upstream `pcm.s`.
Resident PCM is never replaced during play. The streamer writes only an
inactive reserved half; interrupt service does not change the selected PCM
channel/wave bank. All users share the channel-enable shadow in `audio_sub.c`.
This does not qualify arbitrary live-bank replacement.

## The optical drive

CD music and file reads share a pickup. The Lab's policy is:

1. Keep the desired track/pause state as intent.
2. Before a read, lock the drive for data. Finish any in-flight BIOS command,
   stop CD music, then wait for that command's completion.
3. Let the existing scenery reader fill owned 2M Word RAM, or let the voice
   reader fill its reserved Sub staging buffer. They are mutually exclusive
   drive clients. Main renders resident art and can send PCM effects through
   the existing command channel.
4. When the reader is idle, release the drive. Restart the latest requested
   track from its beginning, or leave it stopped if the player requested Stop.
   Desired pause is applied after restart.

This is a deliberate interruption/restart policy, not seamless CD-DA/data
concurrency or resume-at-same-position. Preloading before CD music starts,
using resident PCM during a seek, or the optional long-sample streamer
may suit a game better. Do not issue another BIOS disc command during a live
reader operation. The CD access pump remains serviced through every UI page,
scene fade and gameplay pause.

`audio_drive_next/complete` are portable project policy helpers. The adapters
use actual `BIOS_MSC_*` definitions and poll `BIOS_CDB_CHK` across ticks, without
blocking the foreground in a wait loop. A 30-second example deadline quarantines
a command that never completes. It does not fake BIOS completion or grant a
second drive owner. PCM continues to receive service during a drive fault.

## Main/Sub request channel

The Lab's audio mailbox uses COMCMD4..6 and COMSTAT7; the reader retains
COMCMD0..3 and its existing result fields. COMSTAT4 reports voice bits and drive
state after bootstrap. This explicit register partition is project code, not a
new Megadev protocol. Parameters precede a serial token; acknowledgement and
retirement precede reuse. One queued request is allowed; a busy queue rejects a
new request without overwriting it. Main's lost-ack deadline quarantines the
slot instead of silently freeing it.

Buttons are edge-triggered: holding an audition button does not retrigger on
every frame. Start requests silence for current/desired CD music and PCM voices.
It does not abort a live data read or clear a hardware fault. An acknowledged
request means the Sub manager accepted the message, not that a seek completed
or sound has been heard. Apps needing completion events should expose the
manager's state transition through their existing event system.

The Lab tracks acknowledged command intent; it does not decode every BIOS/media
error status. Production recovery should also interpret `BIOS_CDB_STAT` using
the selected BIOS manual. `CDB_CHK` becoming ready alone is not evidence that a
missing/damaged disc played successfully. The authoring checks establish that
the Lab's two known tracks exist; listening and physical-media failure checks
remain separate evidence.

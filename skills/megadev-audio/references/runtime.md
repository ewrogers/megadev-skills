# Runtime ownership and sound policy

Keep sound requests separate from hardware register access. A request identifies
an effect/track, desired pan/priority and optional lifecycle owner. The sound
manager chooses channels and enforces memory/drive contracts. Sound style,
polyphony and stealing rules belong to the game, not this example.

## Resident PCM

Audio Lab reserves four 4 KiB banks (16 KiB total). It copies samples only at
startup with all channels disabled, using the Sub-visible odd byte lane. Bed,
bell, pluck and drum have separate addresses; a zero/marker tail at 3F00 is
reserved for one-shot silence. The remainder of PCM RAM is unused, not
implicitly available to another owner. No allocator or streamed PCM is supplied.

Hardware channel 1 is the looping bed. Channels 2/3 serve effects; channels 4..8
are unused. This is an illustrative split, not a three-channel hardware limit.
The model selects a free effect slot, otherwise the lower-priority slot;
a lower-priority request cannot steal it. Equal priorities may replace that
slot. Effects have finite release times in a 300 Hz clock shared by PAL/NTSC.
The source sample already fades; expiration explicitly disables the hardware
channel. Bed lifetime is independent. Channel settings preserve the other
enable bits, and pan uses independent left/right four-bit gains.

The Sub foreground loop is the only PCM selector/register owner. It uses the
same four-NOP settings settle interval illustrated in upstream `pcm.s`.
No PCM RAM writes occur while voices play, and interrupt service does not
change the selected PCM channel/wave bank. An asynchronous sample uploader
would need its own range leases and idle-channel handoff; this Lab does not
qualify overwriting a live sample bank.

## The optical drive

CD music and file reads share a pickup. The Lab's policy is:

1. Keep the desired track/pause state as intent.
2. Before a read, lock the drive for data. Finish any in-flight BIOS command,
   stop CD music, then wait for that command's completion.
3. Let the existing reader fill its owned 2M Word RAM buffer. Main renders
   resident art and can send PCM effects through a separate command channel.
4. When the reader is idle, release the drive. Restart the latest requested
   track from its beginning, or leave it stopped if the player requested Stop.
   Desired pause is applied after restart.

This is a deliberate interruption/restart policy, not seamless CD-DA/data
concurrency or resume-at-same-position. Preloading before CD music starts,
using resident PCM during a seek, or a separately designed PCM streaming system
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

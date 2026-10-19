# Playback, clocks and ownership

## SDK facilities versus player policy

The selected SDK provides `filename`, `filebuff`, `filesize`, `access_op` and
`access_op_result` in `lib/sub/cdrom.h`. The Lab uses `CDROM_LOAD_CDC` (direct
load into Sub-accessible memory), waits for `CDROM_IDLE`, then checks
`CDROM_RESULT_OK` (0x64) and exact length. A C ABI shim calls assembly `find_file`
using A0 and carry; packed directory lengths are read at offset 18, not from an
assumed naturally aligned C struct. Publish parameters before `access_op` while
interrupts are masked; do not copy the inline wrapper's ordering blindly into
an asynchronous request. BIOS INT2/CD processing must remain serviced.

Main uses a single project VBlank owner. It uploads before BIOS polling, then calls `BIOS_TRIGGER_IFL2` and
`BIOS_READ_JOYPAD`. `BIOS_DMA_XFER` is wrapped with preserved C callee-saved
registers. Source pointers are Main Work RAM, never stale Word RAM addresses.
PCM uses the actual `sub/pcm.h` register macros. All `ci_*`/`cinema_*` functions,
CIN1 fields and mailboxes are application conventions, not SDK APIs.

Audit against the image source. Milestone source:
[Megadev](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib).
Public build commands follow `ewrogers/megadev:latest`; record its actual revision.

## Fixed producers and consumers

The Sub CPU reads into one of two fixed PRG groups, validates its header, then
copies a requested frame into Sub-owned 2M Word RAM and checks its sum. As soon as the
current group is validated it can read ahead into the *other* PRG group.
CDC never writes the group currently supplying a frame. In-flight prefetch is
reported separately and must drain even if the frame mailbox has already retired.

Main grants Word RAM before publishing a request token. Sub returns ownership,
then publishes result/frame/checksum/echo before the reply token. Main observes
the token, confirms ownership and metadata, copies/checks into its private frame,
then clears the request. No new request reuses that channel until the reply is
zero. A returned next frame may wait in Word RAM while the current frame's
copied upload packets finish. Once the final packet owns its own copy, Main
can receive the next frame into the same private frame buffer while that packet
retires. The next frame becomes current only after the previous flip is acknowledged.
No pointers outlive their ownership grant. A cached next frame also suppresses
further requests until the current page flips; mailbox retirement alone does
not mean the consumer needs another frame. Otherwise it can request the same
frame twice and later reject its duplicate as out of order. If VBlank acknowledges a packet
during a foreground copy, retire that acknowledgement before publishing from
the old timeline state. Otherwise the producer can queue the same flip twice.

The publisher copies at most 4,096 image bytes and one HUD row per early VBlank.
Packets are immutable until acknowledgement. A late or active-display callback
leaves work pending. Only after all bytes arrive in the inactive slot and the
presentation deadline is reached does Main queue the table flip. Register 2's
BIOS cache changes with the real register. All palettes are set with display off;
there are no playback-time CRAM or HBlank effects.

## Timing, pause and audio

The 300 Hz presentation clock advances by five units per NTSC VBlank or six per
PAL VBlank. The nominal image period is 30 units (10/sec). Late frames keep the
previous image and reset the next deadline to actual presentation; they do not
race through a backlog. IMAGES/SEC counts actual page-flip acknowledgements over
one second of foreground time, including stalls; LATE counts delayed presentations.
Neither figure is a cycle-accurate performance profiler. The 256-byte upload
mode intentionally reduces image rate. Display refresh and new image rate differ.

A pause freezes new publication/timeline work and mutes PCM. An already committed
packet/flip can retire; reads continue draining safely. Resume re-enables the
resident score; it is not a sample-accurate music-position resume feature.

The original resident PCM phrase uses eight 4 KiB banks; a short cue uses another.
The 8,192 Hz source uses an approximately matching RF5C164 step (515 from the
12.5 MHz / 384 clock and 11-bit fractional step). Cues are keyed to actual
presented-frame events, not timer guesses. The score is an ambient loop and does
not claim lip sync. Sustained voice should use a dedicated audio-clock policy,
buffered interleaving/refill deadlines and recovery strategy. Share the drive
owner with the audio service rather than starting CD-DA beside data reads.

Skip/error/timeout mutes audio immediately and suppresses new frames. It waits
for matching replies, Word RAM handback and any prefetch CDC operation before
replay can start. No timeout forcibly steals ownership. An indefinitely stalled
reader stays unavailable; production recovery needs an explicit verified reset
path rather than pretending cancellation completed.

The checked copy kernel uses only original-68000 instructions, handles exactly
7,680 aligned bytes and returns a big-endian 16-bit word sum. Sub uses the same
sum before handback; Main compares after copying. The C geometry assertion and
executed guard/ABI tests protect its fixed unrolled bound. This avoids thousands
of per-word helper calls and byte-extraction shifts. Main also consumes VBlanks
that arrived during foreground work before sleeping again; it does not wait an
extra refresh when acknowledgement work is already pending.

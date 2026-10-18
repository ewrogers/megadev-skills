# Records and recovery

## Define bytes, not compiler layout

The Lab's illustrative record is one 64-byte normal BIOS block:

| Offset | Encoding |
| --- | --- |
| 0–7 | Fixed family magic `MDSAVE01` |
| 8–9 | Big-endian format version (1 or 2) |
| 10–11 | Payload length (4 for v1, 6 for v2) |
| 12–15 | Nonzero, big-endian sequence number |
| 16–19 | Level and coins, big-endian 16-bit values |
| 20–21 | Flags in v2; zero in v1 |
| Remaining bytes through 59 | Reserved, zero |
| 60–63 | Big-endian CRC-32 of bytes 0–59 |

Semantic validation restricts the Lab fields to level 1–9, coins 0–999 and flags
0–3. Adapt those to the actual game. CRC detects accidental corruption; it is
not authentication. Check the complete record before mutating live state.
Decoding v1 supplies flags=0 in RAM. The next explicit save writes v2 into the
replacement slot; it does not silently rewrite the source during a load.

## Two independent named records

`MDSAVELAB_A` and `MDSAVELAB_B` are the Lab's private BIOS names. Two files consume
128 payload bytes plus BIOS filesystem overhead, not an entire game-wide RAM
reservation. Choose another project-specific namespace for an actual game.

Scan both slots. Choose the greatest valid sequence. If the other slot is
corrupt, load the verified survivor and report recovery. Missing data is distinct
from corrupt data. If neither slot validates, retain the game's current values
and report failure; do not invent a successful load.

A save scans first, then writes the slot other than the newest valid one. It
compares all 64 readback bytes before reporting success. If a write tears or
verification fails, the previous record has not deliberately been overwritten.
Future-version, unexpected size/mode, read-error and conflicting equal-sequence
records block writes rather than letting an older program destroy newer data.
Sequence exhaustion refuses further saves instead of wrapping into stale order.

This reduces record-level failure risk; it is not a guarantee against a dead
battery, filesystem corruption or power failure damaging BIOS metadata shared by
both files. Backups and device-level qualification remain separate concerns.
Do not implement a failed update as delete-old-then-create-new.

## Service lifetime and UI

One Sub foreground service owns the BIOS workspace and performs save operations.
Main writes parameters first and the request token last. Sub publishes values and
status before the matching acknowledgment. Main consumes that snapshot, clears
its command and waits for reply retirement before reusing the slot. A 30-second
lost-reply deadline quarantines it; it does not cancel a BIOS write or claim it
has finished. Continue the existing Sub INT2/CD service through menus.

Show success only after readback. Report unavailable media, failed writes/check
space, incompatible records and recovery in language the player can act on.
Do not equate an emulator state snapshot with an on-media save. Do not freeze
gameplay's entire service layer merely because a save UI is open.

The recovery demonstration requires two valid Lab records and an explicit
Help-page A command. It corrupts only the newest Lab record's CRC and then
loads the older record. It does not corrupt arbitrary RAM, format media or
delete other games' files. Keep such diagnostics out of normal shipped controls.

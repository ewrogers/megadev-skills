# Replay contracts

## Define the reproducible state

Record a schema, logical content version, initial authoritative state and PRNG seed,
logical tick duration, input mapping/device mode and the exact build/assets identity.
Include inventory, timers, previous held buttons, pending gameplay requests and
other hidden state if they affect future decisions. A level number and seed alone
are rarely sufficient for a mid-game replay.

Test Lab starts from a known fixture snapshot. Its in-memory `Replay` uses **5824
bytes**: two 20-byte states, 720 eight-byte samples, two 32-bit diagnostic hashes
and sixteen bytes of metadata. This is a 12-second tape at 60 logical ticks/sec.
It is not a disk/file ABI and is never a raw C struct save format. A game can
choose shorter tapes, RLE input or sparse checkpoints after measuring the trade-off.
Use explicit endian fields, lengths and integrity checks if exporting a tape.

The fixture content value `RP_CONTENT` identifies the logical example, not a
cryptographic asset digest. Keep real disc, module and asset hashes in the QA
manifest. Do not silently update expected hashes when changing gameplay; review
why the old expectation changed.

## Sample input and external decisions

The Lab adapts actual Megadev D-pad masks from `bios_joy1_hold`. Record/stop/replay
buttons control the harness and stay outside the simulation. There are no gameplay
button edges in this course. For another game, record pressed/released actions or
derive them from held buttons with the previous state in the initial snapshot.
Sample once per logical tick; if one video frame services multiple ticks, define
how the sampled input applies to each. Human inputs need not quantize identically
on PAL and NTSC even when a saved logical tape replays identically.

Each Lab sample includes an event as well as input and the expected state hash.
The course uses a clearly controlled **fixture** gate-ready event at logical tick
180. It does not get that event from a live CD reader. Playback consumes the tape's
events; real drive completion order is not rerun. A real game would capture its
service result/token at the gameplay boundary and feed that outcome into playback.
Never apply a stale live reply on top of a recorded completion.

## Compare without unstable bytes

`rp_hash` mixes explicit fields with unsigned 32-bit arithmetic. It is a diagnostic
checksum, not cryptographic proof. Include all authoritative fields and add direct
assertions for meaningful outcomes. Do not hash struct padding, C pointers, host
allocation addresses, render caches or incidental scheduling counters. Keep sound
and cosmetic randomness separate from the game's PRNG stream.

The Lab stops at the first mismatch and preserves zero-based tick, expected hash
and actual hash. The tape is unchanged. Turning on the deliberate speed bug before
replaying a clean movement tape demonstrates divergence. If the tape never moves,
that mutation has no observable effect; the negative-control fixture always moves.

Before playback, reject empty/oversized tapes, unknown schema/content and invalid
input/event fields. A full tape stops at capacity. Starting a new recording clears
the prior logical recording explicitly; the example has no disk persistence.
The Lab's initialized recorder owns its counters; it is not a parser for arbitrary
corrupt in-memory structures. Imported tapes need a separate checked decoder.

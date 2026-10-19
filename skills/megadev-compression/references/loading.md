# Loading and RAM trade-offs

## Choose the pipeline before the codec

Compression helps when saved transfer/storage is worth decoding and working
memory. Use per-asset results. A small file can occupy the same number of
2048-byte sectors after compression; incompressible data can grow. Neither file
ratio nor available CD capacity proves a loading speedup.

Separate these costs:

1. File lookup/seek/read into a sector-rounded input reservation.
2. Format validation and decoder workspace/history requirements.
3. Decoded output, which may coexist with the input and old scene resources.
4. Asset integrity checks, transfer to VRAM and visible publication.

The final tile/palette/sprite budgets do not shrink because disc bytes are
compressed. Sector rounding must fit the **writable** input capacity even when
only logical file bytes are parsed. Check directory size before starting CDC;
an on-disc header cannot authorize an oversized write.

## Lab ownership

The existing bounded reader protocol is reused, with a compression stage added
before its reply. No second mailbox or competing drive owner is installed.

- Input occupies Sub-visible 2M Word RAM `080000–087FFF` (32 KiB).
- Output occupies `090000–097FFF` (32 KiB).
- Main sees the same output at `210000–217FFF` after ownership returns.
- Both reservations remain disjoint from module code and both CPU stacks.
- Sub Program RAM holds only small policy state, tables and code; there is no
  full additional asset copy there.

Sub starts `CDROM_LOAD_CDC` only with exclusive 2M ownership. It waits for actual
`CDROM_IDLE`, checks result and logical size, validates the compressed stream,
then decodes to output. The raw baseline copies from the same input reservation
to the same output reservation. Source bytes remain immutable throughout.

Only after decoding returns does Sub grant Word RAM to Main and publish its
reply token after the metadata. Main retires the mailbox, verifies the MDA
content incrementally and uploads copied chunks through its resident VBlank
publisher. It never queues pointers into Word RAM across ownership changes.

Cancellation and the 30-second example read deadline mark the result unwanted;
they do not interrupt a live CDC transfer or preempt the assembly decoder.
The request slot and input/output leases remain occupied until the old work
retires. A late result must not become the newly requested scene. A permanently
stuck read needs recovery/reset policy, not speculative buffer reuse.

For audio integration, use the game's existing drive arbiter. A compressed data
read still seeks and competes with CD-DA; compression does not make those two
operations independent. The Lab contains no separate audio driver.

## Peak versus reserved RAM

`active input = ceil(file_bytes / 2048) * 2048`.
The Lab reports that input plus the decoded bytes simultaneously live. It also
records the fixed 64 KiB reservation separately. Smaller compressed input does
not automatically release part of that fixed reservation to an allocator.

A game can choose a tighter input pool based on its largest rounded compressed
file, if its ownership/lifetime rules allow it. Conversely, direct raw loading
into final RAM can avoid the separate encoded input reservation. Do not teach
compression as universally saving RAM: some pipelines require more peak memory.

Preloading retains the old visible picture/resources while preparing the next.
Count those overlapping lifetimes, both VRAM slots and any decoder work buffers.
After the new asset is ready, the transition can consume it without another
read/decode. Preloading hides latency; it does not eliminate the underlying work.

## CPU responsiveness

Kosinski runs synchronously in Sub foreground with interrupt service enabled.
Main keeps animating/rendering, and the SDK's Sub INT2 service remains installed.
This is bounded input/output, **not** an incremental decoder with a guaranteed
per-frame CPU allowance. Large jobs can delay other Sub foreground work.

If a game's deadline cannot tolerate a monolithic decode, choose independently
compressed chunks or a separately qualified resumable decoder. Reserve state
across yields, prove source/output/history lifetimes and measure the result.
Do not drop SGDK's task/decompression APIs into Megadev or move decoding into a
VBlank callback to make it appear asynchronous.

# Formats and authoring

Audited SDK: `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` in
`ewrogers/megadev:latest`. Reinspect the selected image when it changes.

## Actual Megadev support

- [`lib/kos_cmp.s`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/kos_cmp.s)
  exports `Kos_Decomp`, taking A0 source and A1 output. It is a byte-oriented
  Kosinski decoder with literals, short/long history references and an end
  marker. It has no source length or destination capacity argument.
- [`lib/kos_cmp.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/kos_cmp.h)
  declares `dcmp_kosinski`, but its audited inline assembly uses operand `%p0`
  for the call even though operand 0 is the A0 output, and omits some clobbers
  and a memory barrier. The Lab uses an explicit C-ABI assembly adapter instead
  of teaching that wrapper as verified. Re-audit later SDK revisions.
- [`main/bios.def.h`](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h)
  documents `BIOS_GFX_DECOMP` (Nemesis to VRAM), `BIOS_GFX_DECOMP_RAM` and
  `BIOS_MAP_DECOMP`. Check the exact register/workspace contracts for that
  separate path. These are BIOS graphics services, not generic safe codecs.
  The Lab does not execute or qualify Nemesis/Enigma.

Compression is software, not a Sega CD hardware decompression engine. The
scaling ASIC is not an arbitrary compressed-file decoder. Do not substitute
SGDK's APLIB/LZ4W APIs or ResComp declarations without explicitly integrating
and qualifying a separate implementation.

## Kosinski safety boundary

The Lab's `cp_kos_valid` walks the **complete** source without producing output:

- Every descriptor, literal, offset and extended-length read must fit.
- Every history reference points into bytes already produced; overlapping
  references are valid and are used for repeated runs.
- Every token fits the independently known decoded length and output capacity.
- The terminator occurs exactly at the expected output size and file end.
  Sector padding is outside the logical file length and is not parsed as data.

A descriptor is little-endian and consumed least-significant bit first. The
SDK reloads the next descriptor immediately after consuming bit 16, **before**
reading that token's payload. This is easy to get wrong when writing an encoder
or validator. Short references copy 2–5 bytes with a negative 8-bit offset;
long references use a negative 13-bit offset. Extended lengths include stop and
no-output continuation tokens. Compare against the real assembly, not just a
Python round trip that might repeat the same bug.

`kos_bridge.s` preserves the C callee-saved registers around `Kos_Decomp`.
Validation and decoding occur while Sub exclusively owns immutable input and
a disjoint output range. Do not run the stock decoder on untrusted bytes merely
because a file header claims a reasonable decoded size. Do not rely on the
terminator alone for memory safety.

Successful format validation is not content integrity. The example validates
the resulting MDA header, palette, map references and CRC32 before upload.
Choose the equivalent integrity check for the game's real asset format.

## Authoring

`tools/make_compressed.py` supplies a deterministic, standard-library Python
encoder for this audited Kosinski dialect. It uses literals and long references
with an 8 KiB history and limits match-search candidates; it is an illustrative
encoder, not a claim of optimal compression. It emits:

- Raw and packed files in the ISO's data directory.
- Independently generated known source/output lengths.
- A JSON report of file bytes, sectors, output size, active buffer bytes,
  reserved buffer capacity and payload hashes.

The garden/cavern are prepared native assets from the repository's Asset Lab.
The noise case randomizes only pattern bytes and recomputes the asset CRC; its
random dots are deliberate, not a decoder fault. Substitute original project
assets without adopting the Lab's look or file format.

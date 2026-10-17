# MDA1 teaching format and checks

This small container belongs to the example, not Megadev or SGDK. Existing projects may retain raw `.chr`/`.pal` resources or another verified layout. The helpers support one palette per asset, backgrounds up to 64×32 cells, and regular sheets whose hardware-sprite frames are 8–32 pixels per axis in steps of eight. Input is limited to 512×512; split larger content deliberately.

## Binary layout

All numbers are big-endian. No C struct is cast over the serialized header.

| Offset | Field |
| --- | --- |
| 0 | ASCII MDA1 |
| 4 | u16 version = 1 |
| 6 | u16 kind: 0 background, 1 sprite sheet |
| 8, 10 | u16 width/height in 8×8 cells; frame dimensions for a sheet |
| 12 | u16 pattern count, 1..2048 |
| 14 | u16 entries: map cells or frame count |
| 16 | u32 pattern bytes = pattern count × 32 |
| 20 | u32 total file bytes |
| 24 | u32 CRC32 of all bytes after the header |
| 28 | u32 reserved = 0 |
| 32 | 16 CRAM words (32 bytes) |
| 64 | Packed patterns, followed by map words for backgrounds |

An 8×8 pattern contains 32 bytes: each byte holds the left pixel in the high nibble and the next pixel in the low nibble. Background maps have one word per source cell in row order. Bits 0–10 are relative pattern indices, bit 11 horizontal flip, bit 12 vertical flip. Palette and priority bits must be absent; runtime assigns those alongside the VRAM base. Validate the final base-plus-index range too.

Background deduplication canonicalizes horizontal/vertical flips. Sprite sheets deliberately do not deduplicate: each frame is contiguous and its tiles run by columns, as hardware sprite rendering expects. Frame cells themselves remain in source-sheet row order. The report records source columns for preview reconstruction; runtime sees a linear frame list.

The 2048-pattern format ceiling is not a promise that an asset fits VRAM: tables, resident art and addressing reservations reduce the available space. Asset Lab uses a smaller 768-pattern scene region.

## Validation layers

The standard-library inspector checks header/version, lengths, geometry, CRC, legal CRAM bits and relative map references. The C decoder performs corresponding bounded byte reads before upload. The available length comes from a verified build manifest in the Lab, not the untrusted file header. Disc reads may include sector padding; the parser validates the exact file payload length.

CRC catches accidental payload corruption. It is not authentication, and a format parser does not establish CPU ownership, DMA safety or address layout. Reject bad assets before touching the VDP. Keep uncertain loader buffers unavailable after a timeout.

The example's CRC routine is synchronous loading-phase work. It is not a frame-bounded streaming decoder. The checked Megadev make rules link directly with `ld`, without automatically supplying a general C runtime/libgcc. The parser uses small bounded shift/add header arithmetic so both release and debug builds link without hidden multiply-helper dependencies. This is an example constraint, not advice to replace every C multiplication in a game.

Tests round-trip source pixels, verify frame/tile ordering and endianness, reject malformed headers/maps/CRC, exercise odd-address input safely in host C, and execute the compiled decoder on an original M68000 model. Native ARM64/AMD64 CI regenerates prepared bytes and compares built payloads. Those checks do not emulate BIOS DMA or physical display timing.

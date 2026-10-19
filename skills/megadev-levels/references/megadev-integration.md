# Megadev rendering and residency

The Lab was checked against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` using `ewrogers/megadev:latest`. It reads BIOS joypad state from `main/bios.h`, uses button masks from `main/io.def.h`, and uses VDP definitions from `main/vdp.h` and `main/memmap.h`. Verify the selected SDK source when changing an adapter. No SGDK MAP, SPR or resource compiler API is assumed.

## Two planes and a bounded cache

The plane tables remain at 0xC000 (A) and 0xE000 (B). Both are explicitly configured to **64×32 hardware cells** with VDP register 16 set to 0x01. The CPU cache uses the same dimensions. Retaining a taller BIOS default would expose unfilled rows after vertical wrapping, even though the host cache model passed.

Plane B draws Ground; Plane A draws Foreground. Pattern zero is uploaded as fully transparent and represents empty world cells. The font SPACE pattern contains opaque HUD background pixels and must never be used as an empty foreground tile. High-priority canopy tiles can cover low-priority actors; this simple example is not a general depth-sorting system.

Each authoring tile becomes four 8×8 patterns. Incoming cache columns have 32 words per plane; rows have 64. At most one row and one column change on each plane per presentation: four runs, 192 words. Runs are arranged in physical VRAM order, with word data selected from the corresponding world coordinates. VDP auto-increment is 128 for columns and 2 for rows, then restored to 2.

Two immutable 2,064-byte frames hold sprites, four HUD rows, camera coordinates and patch runs. The callback validates counts, offsets, strides and plane addresses before writing VDP. It holds a late frame for the next early VBlank. The data ceiling is 776 bytes (eight SAT entries, 320 HUD bytes, eight scroll bytes and 384 tile bytes); command/register writes also cost time and are not included in that byte count. Ordinary frames do not write CRAM.

VRAM allocations: transparent tile at 0, font at 0x0400, 49 actor patterns at 0x1000, up to 256 terrain patterns at 0x2000, Window at 0xA000, SAT at 0xB800, HScroll at 0xBC00 and the two plane tables. The Lab uses 128 terrain patterns and all four palette lines. Increasing world dimensions does not increase resident VRAM art automatically.

## Memory and loading ownership

The two 768×512-pixel areas use a 15,572-byte prepared world bundle resident in Main RAM. All art is shared and resident. The disc startup loads the application through Megadev's MMD bootstrap; there is no new per-frame CD reader, Word RAM consumer or Sub service. Sub completes startup and the existing INT2 access continuation remains intact.

This is intentionally a small resident profile, not a claim of CD streaming on every crossing. `megadev-streaming` owns larger-area loading/preloading and safe Word RAM handoffs. The supplied `lv_open` accepts a complete resident bundle; it cannot consume partial CD chunks. A streamed profile needs retained graph identities, a complete validated area payload and a lifetime-safe area view before activation. Put that adapter behind the existing streaming owner, validate the complete destination before activation, and retain the old area until readers/jobs are drained. Keep character/UI art resident while replacing area-specific patterns/maps at a bounded transition point. Use `megadev-memory` and linked map reports for the real RAM budget; the linker's reservations reject overgrown modules.

The game model follows the gameplay timing contract: a 300 Hz accumulator consumes five units per 60 Hz step, with four-step catch-up and pause semantics. Main owns collision/cameras; the renderer only samples completed state. Short standalone PSG cues have one owner; a game with an audio service should route those requests through that service. Do not add a competing PSG writer.

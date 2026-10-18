# ASIC layouts and projection

## Verified API boundary

Megadev revision `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` supplies register
addresses in `lib/sub/gate_arr.def.h` and C macros in `lib/sub/gate_arr.h`.
The ASIC macros there are **already dereferenced lvalues**, unlike the pointer
macros for `ga_reg_memmode` and communication words. Inspect the actual header;
adding `*` uniformly is incorrect. There is no SDK `rotate_sprite()` abstraction.

The Lab accesses the ASIC from Sub foreground and polls completion. It masks
only ASIC INT1 in `GA_REG_INTMASK` before starting, preserving the other interrupt
bits and Megadev's existing Sub INT2/CD service and Main VBlank ownership. Do not
combine this polling owner with a separate ASIC interrupt-driven owner.

## Fixed Lab profile

The deliberately narrow profile is 16×16-pixel stamps, a 256×256-pixel repeating
map, overwrite priority, and a 128×128-pixel output. The ASIC requires 2M Word RAM;
1M mode is not an alternative backing layout for the operation.

All offsets below are byte offsets within the 256 KiB Word RAM region. Main's
linear view is at `WORD_RAM`, Sub's at `WORD_RAM_2M`.

| Region | Offset | Bytes | Contract |
| --- | --- | --- | --- |
| Stamp art | 0 | 32,896 | Blank stamp 0, then 256 native 16px stamps |
| Map | 0x9000 | 512 | 16×16 big-endian stamp-index words; 512-byte alignment |
| Trace | 0x9400 | 1,024 | 128 lines × four words; 8-byte alignment |
| Output | 0x10000 | 8,192 | 128×128 packed 4bpp; 32-byte alignment |

This is a demonstrator's partition, not a universal reservation for every game.
The selected sizes/alignments depend on stamp and map modes. In particular, the
SDK bit named `16x16_SCREEN` is a map-mode bit, not the output image's resolution.
Do not infer hardware geometry solely from a macro's name.

Each 16px stamp is 128 bytes: the left 8×16 strip followed by the right 8×16
strip, high nibble first. This is four 8px VDP patterns in column order. Map index
zero yields transparent output; the first useful stamp is index 1. Map bits
0–10 select the stamp, bits 13/14 encode quarter/half rotations and bit 15 flips
horizontally; the Lab uses only plain indices. These bits are not palette selectors.
32px stamps and 4096px maps need their own alignment/index validation and are not
implemented by this fixed example.

A trace line is four big-endian 16-bit words: source X, source Y, X step, Y step.
Starts use 13.3 fixed point; steps are signed 5.11. One source pixel per output
pixel is step 2048. Hardware accumulates steps for every pixel. Repeat wraps the
selected map extent; without repeat, out-of-map samples are transparent.
Converting a negative start to an unsigned word is intentional modulo encoding.
The Lab uses truncating C division when reducing precision, not an assumed
arithmetic right shift of negative values.

## Sub registers and trigger

| Address / Megadev macro | Lab value | Units |
| --- | --- | --- |
| FF8058 `ga_reg_stampsize` | 1 | Repeat, 16px stamps, 256px map; bit 15 reads active |
| FF805A `ga_reg_stampmapbase` | 0x9000 / 4 | Map byte offset divided by four |
| FF805C `ga_reg_imgbufvsize` | 15 | Vertical cells minus one, 128/8−1 |
| FF805E `ga_reg_imgbufstart` | 0x10000 / 4 | Image byte offset divided by four |
| FF8060 `ga_reg_imgbufoffset` | 0 | Initial pixel offset within cell |
| FF8062 `ga_reg_imgbufhdotsize` | 128 | Horizontal output pixels |
| FF8064 `ga_reg_imgbufvdotsize` | 128 | Output scanlines; not a completion token |
| FF8066 `ga_reg_tracevectbase` | 0x9400 / 4 | Trace offset divided by four; **write starts operation** |

Set overwrite priority in MEMMODE without changing ownership/layout bits. The
Lab initializes every input and writes trace base last. Poll bit 15 of FF8058;
only then return Word RAM to Main. Do not overwrite trace/configuration mid-run.

Output consists of 8px-wide vertical strips. A column occupies `(vcells+1)*32`
bytes, so the name-table pattern for output cell `(x,y)` is `base+x*16+y` in this
profile. Treating it as row-major VDP tiles produces scrambled imagery.

## Rotation, zoom and floor

An inverse transform maps each destination pixel to a source position. A sine
lookup with integer interpolation supplies 256 heading phases and the row's start and X/Y steps; smaller step magnifies the art.
A quarter turn changes the step direction, not the VDP tile-flip flags.

For the floor, the Lab chooses a reciprocal depth for each scanline below an
implicit horizon. Distant rows use larger sampling steps; nearer rows use smaller
ones. Heading rotates both the starting position and line direction. Forward
motion moves the source center. The denominator never reaches zero. Clamp or
reject horizons/steps in a more general camera instead of overflowing fixed point.
This is a flat repeating ground-plane illusion, not elevation or occlusion logic.

`scale.c` has a host-C multiply and a target original-68000 `muls.w` primitive.
The latter prevents optimizer reassociation into unsupported 32×32 helper calls
with Megadev's direct module link. Both implement signed 16×16→32 multiplication;
callers prove operand/product bounds. The inverse-depth table also avoids a
runtime wide divide. Do not assume this local choice is necessary for all C math.

## Sources

- [Megadev register definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.def.h)
- [Megadev C register macros](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.h)
- [Ares v148 ASIC implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/gpu.cpp), independently checked for sampling, stamp and output ordering.
- [Ares v148 register decoding](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/io-internal.cpp), checked for register units, masks and write-to-start behavior.

Emulator code supports reproducible implementation checks, not a claim of
physical timing qualification. No emulator implementation code is copied into
the Lab; its portable policy and art are original MIT project code.

---
name: megadev-math
description: Choose and validate fixed-point formats, vectors, trig and interpolation for Megadev on the original Mega CD 68000 CPUs. Use for fractional movement, direction normalization, rotations, easing, overflow or numeric-range bugs; includes a standalone Math Lab and executable checks.
---

# Megadev Math

Start with the game's units and maximum values. Choose the format from those requirements, then prove every intermediate fits. The Lab's orbit display, colors, controls and motion are illustrations, not design defaults.

## Workflow

1. Inspect the project's Megadev revision, compiler flags and existing numeric helpers. Read actual `lib/math.h` and `lib/fixed.h`; names alone are not an API contract. See [the SDK audit and arithmetic contracts](references/arithmetic.md).
2. Write down position/velocity units, fractional bits, maximum delta, product width and rounding rule. Signed Q8.8 is useful for local offsets but cannot hold an absolute 320-pixel screen coordinate. Use a wider position or an integer origin plus a bounded offset.
3. Use checked operations at trust boundaries. Reject overflow or zero division before executing a trapping instruction, and preserve the old result on failure. Do not turn a numerical failure into an accidental wraparound teleport.
4. Select the cheapest sufficient geometric operation. Compare squared distances for radius tests; normalize only when a unit direction is required. See [vectors and trig](references/vectors.md).
5. Separate time from interpolation progress. Keep authoritative state in the fixed game tick, advance render clocks through the existing timing owner, and specify endpoint/interrupt behavior. See [interpolation and integration](references/integration.md).
6. Run independent host oracles and compiled original-M68000 execution checks. Then inspect PAL/NTSC motion in ares. Emulator success does not qualify real-hardware timing.

## Standalone example

Run `scripts/create_project.py` with a new destination. It copies [Math Lab](assets/math-lab/README.md), including application helpers, generated quarter-wave table, procedural art, host checks and a Megadev boot/module shell. It refuses an existing destination.

Use `ewrogers/megadev:latest` with explicit `linux/arm64` or `linux/amd64`, `--cpus=2`, a bind mount and at most two make jobs. Follow the Lab README for complete commands. No SGDK library, `fix16`, `sinFix16` or floating-point runtime is assumed.

## Ownership and scope

- `math_core.c` owns numeric contracts only. It has no VDP, CD, mailbox, allocation or timing side effects.
- `motion.c` demonstrates consuming those helpers; its speed and ranges are sample choices.
- Reuse the project's renderer, input snapshot and time accumulator. Do not install another VBlank callback just to use math.
- Both CPUs are original 68000s; the Sub CPU is not a floating-point or vector coprocessor. Moving arithmetic there introduces coordination and Word RAM ownership costs. Use `megadev-system` and `megadev-memory` for those contracts.
- Leave collision policy to `megadev-collision`, gameplay timing to `megadev-gameplay`, ASIC image transforms to `megadev-scaling` and measurements to `megadev-performance`. Trig-rotating a coordinate is not rendering a rotated sprite.

## Acceptance

Require boundary and negative inputs, failure preservation, tiny/zero vector handling, all trig quadrants, interpolation endpoints, and PAL/NTSC equal logical time. Use the selected target flags, including debug, because host arithmetic cannot validate inline assembly. Record the SDK revision and known API limitations with the results.

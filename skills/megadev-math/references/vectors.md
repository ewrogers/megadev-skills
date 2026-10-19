# Vectors, trig and bounded cost

Use integer squared-distance comparisons when a test only asks whether a point is within a radius. Square the radius in the same raw units and use an unsigned type large enough for the sum. Clamp world deltas or widen their storage before subtracting; two valid Q8.8 positions can have a difference outside Q8.8.

`mm_isqrt` computes floor(sqrt(n)) with sixteen restoring steps over an unsigned 32-bit input. It does not divide. Normalization first scales both magnitudes up together until at least one reaches 16384, then finds the integer length and divides each component. This avoids `(1,1)` becoming `(256,256)` due to floor(sqrt(2))=1. Zero vectors fail without replacing the caller's direction. Decide whether a game retains its last facing, stops or chooses a fallback when normalization fails.

The result is approximate: component error in the tested full-range cases is within 1.02 raw Q8.8 units. That is test evidence, not a universal exact-unit proof. Quantization can shorten or slightly lengthen a vector; don't promise an exact speed cap from normalization alone. Handle arrival separately to avoid overshoot/oscillation. Math Lab snaps near its target and uses deliberately bounded deltas; these are not production pathfinding rules.

The quarter-wave table has 257 unsigned words: **514 bytes**, covering sin from 0 to one quarter-turn, inclusive. Quadrant reflection/sign recovers a 1024-step circle. `mm_cos(a)` is `mm_sin(a+256)`. Values are rounded to nearest Q8.8, with an all-angle independent oracle checking error ≤0.501 raw units. Angles wrap intentionally; coordinate overflow does not.

`mm_rotate` evaluates `(x*c−y*s, x*s+y*c)`, rescales toward zero, checks both components, then commits both. A vector whose components fit before rotation might not fit afterward; rejection leaves its output intact. Repeatedly rotating an already rounded vector accumulates drift. For stable orbits rotate a saved base vector by an absolute phase, as the Lab does.

Original-M68000 word multiply is suitable for these bounded products; division and normalization cost more than a table lookup or squared-distance test. Reuse directions, normalize on decisions rather than every pair every frame, and precompute static paths/table values when useful. Measure the generated target path before replacing clear C with assembly. The Lab makes no cycle-accurate benchmark claim.

For wider worlds choose a different format deliberately. Signed 32-bit Q16.16 provides wider coordinates, but multiplying two arbitrary full-range values requires a wider intermediate or an explicitly bounded/narrowed operand. Do not mechanically change typedefs and assume the current 16×16→32 proofs still apply.

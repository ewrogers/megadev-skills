# Bounds and sliding

## Local arithmetic contract

The Lab checks origins and end origins within ±16,384 raw units (±1,024 px), positive extents up to 4,096 units (256 px), and each body's displacement within ±4,096 per axis. Relative displacement can reach ±8,192. Box far edges can extend beyond the origin range; intermediate edges use signed 32-bit arithmetic.

Translate a large world into a local collision region before querying. Reject out-of-range motion or split it into bounded intervals while preserving ordering and time. Never narrow a large value to 16 bits before checking it.

Slab numerators have magnitude at most 36,864 and denominators at most 8,192. Cross-products fit signed 32 bits. Exact time comparison therefore needs neither floating point nor 64-bit division. Display-time numerators also fit 32 bits; their quotient is at most 4,096.

The target uses base-68000 `mulu.w` and `divu.w` for validated factors and quotients. Sign restoration is explicit. These private helpers are not general 32-bit multiplication/division APIs. The two-instruction interpolation helper uses an early-clobber output constraint so the divisor cannot alias an output overwritten by multiplication. Retain these bounds and target tests when changing precision.

The broad-phase helper is kept out of line. A fully inlined optimized variant failed an inside-point target regression; the host version passed. Do not remove this boundary based only on host results. The current tests compare target results to a Fraction oracle as well as checking debug/release behavior.

## Bounded work

A query accepts at most 32 candidates. It validates the complete list before broad-phase rejection. A null list is valid only with count zero. Larger worlds should gather a small nearby list from a tile grid/spatial index, with an explicit overflow path. Never omit a solid to fit the list. `visited` and `narrow` counters distinguish candidate work from exact tests; they are not timing measurements.

Sliding performs at most three casts. Each hit advances by the exact fraction, truncating any tangential sub-unit remainder toward zero, then removes blocked-axis motion. A miss consumes the rest. Initial overlap or invalid input preserves unresolved movement; exhaustion returns it explicitly. Check the result instead of assuming the returned box consumed the whole request.

For a production mover, define spawn validation, recovery from overlaps and what to do with unresolved movement. Use deterministic fallback behavior and record capacity failures. This example does not push other bodies, resolve penetrations or normalize diagonal input. Its fast diagonal mode deliberately stresses corners.

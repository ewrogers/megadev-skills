# Arithmetic contracts and actual SDK behavior

Source audit: Megadev revision `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, as selected by `ewrogers/megadev:latest` during this work. Reinspect a newer image: `latest` is a moving tag. The upstream source is [drojaazu/megadev](https://github.com/drojaazu/megadev), specifically `lib/math.h` and `lib/fixed.h`. The container's `doctor` output records the actual compiler/SDK and original-M68000 flags.

## SDK findings

`mul(s16,s16)` and `mulu(u16,u16)` return 32-bit word products using `MULS.W`/`MULU.W`. Math Lab calls these actual SDK helpers on the target; its host branch uses widened C multiplication. Target instruction tests validate the selected compiler's interpretation of those inline-assembly constraints.

At this revision, `divu` and `div` each assign `out.quot` twice, leaving `rem` uninitialized; the purported signed wrapper also uses `DIVU.W`. Several conversion macros in `fixed.h` contain an extra closing parenthesis and narrow before scaling. Do not teach them as working APIs or silently depend on them. The Lab supplies explicitly named **application** helpers for conversions and division; it does not patch the SDK. Other rounding helpers were not qualified here.

The SDK's `f16`/`f32` aliases use 8/16 fractional bits respectively. Those names are not SGDK's types. Never infer interchangeable representation from a similarly named API in another SDK.

## Sample representation

| Quantity | Stored form | Meaning / bound |
| --- | --- | --- |
| Local coordinate | signed 16-bit Q8.8 | raw / 256, −128 to 127.99609375 |
| Unit vector | two Q8.8 components | approximately length 1; quantized |
| Squared length | unsigned 32-bit Q16.16 | sum of squares, up to 2147483648 raw |
| Dot product | checked signed 32-bit Q16.16 | reject an overflowing sum |
| Angle | unsigned units modulo 1024 | 0 right, 256 down in screen coordinates |
| Progress | unsigned 0..256 | 0 start, 256 end |

A raw Q8.8 product is Q16.16: divide it by 256 before storing Q8.8. Widen operands **before** the operation. A 32-bit sum of two word products can still overflow; `mm_dot` explicitly guards it. `mm_length2` requires a non-null vector and returns an unsigned sum so both components at −32768 are representable.

All checked helpers return 1 on success, 0 on failure, and preserve the output on failure. Null outputs are failures. Vector input/output aliasing is supported for normalization and rotation; invalid vector pointers are rejected. `mm_to_int` truncates toward zero, not floor; −255 raw becomes 0. Multiplication, division and interpolation also truncate fractional changes toward zero. Do not use this conversion to choose negative-world map cells, which often need floor division instead.

`mm_div` forms `abs(a)*256`, checks zero divisor and the permitted signed quotient, then executes one private `DIVU.W`. Its callers prove the unsigned quotient fits before the instruction; checking afterward cannot prevent a divide exception. Minimum negative values are widened before negating. No division occurs in an interrupt.

Avoid left-shifting negative signed values, signed overflow, assumptions about signed right shift and unsafe `abs(-32768)`. The helpers use unsigned magnitude operations and checked narrowing. Floating point is used only by the host table generator, never in the target runtime. Keep divisors, range and rounding semantics in public contracts rather than relying on compiler optimization behavior.

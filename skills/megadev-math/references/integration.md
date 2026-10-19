# Interpolation, time and validation

`mm_lerp(a,b,t)` uses progress 0..256. It supports the entire signed-word endpoint range by widening the endpoint difference, multiplying its unsigned magnitude by `t`, truncating, then adding the signed step to `a`. Both endpoints are exact. Equal endpoints remain equal. Invalid progress fails; it is not silently clamped.

`mm_smoothstep(t)` evaluates the cubic `t²(3−2t)` at the same progress scale with integer intermediates; its integer result is monotonic and both endpoints are exact. Its easing is an option for presentation, not a recommended feel for every game. Use linear motion, a designer-authored curve or another easing when the brief needs it.

Keep authoritative tick state separate from interpolation. Math Lab advances 60 logical ticks per second: each NTSC VBlank adds 5 clock units, each PAL VBlank adds 6, and each simulation tick consumes 5. Elapsed work is capped at four ticks per update and excess time is counted/dropped. This demonstrates a bounded overload policy, not a claim that every game should run at 60 logical Hz. The existing gameplay timing owner should make that choice. A renderer can interpolate between snapshots if smoother PAL presentation is desired; this Lab does not do so.

Pausing clears the partial clock and freezes authoritative motion; resuming does not replay accumulated wall time. Reset replaces the local state. When changing view, the Lab preserves its current motion clock. Pressing A reverses the orbit or easing direction, or switches automatic target movement in the vector view. These controls are sample UI only.

## Resource and publication ownership

Numeric helpers allocate nothing. `MmVec` is 4 bytes and `MmGame` is 30 bytes; the quarter-wave table is 514 bytes of resident module data. The sample renderer reserves two 1640-byte frame buffers and caps sprites at 28. Each ordinary upload is at most 544 data bytes: 28 SAT entries plus four 40-cell HUD rows. These are data counts, not a whole-system RAM total or a timing guarantee.

Foreground code publishes an immutable frame; VBlank consumes it only in the early blank window. Invalid buffer/count metadata fails closed, late publication is deferred, and ordinary updates do not touch CRAM. Palettes are installed with display off at startup. Math does not own the VDP or Main/Sub mailbox. The unchanged room/Sub module shell only proves standalone module startup; the math itself executes on Main, with no fabricated Sub acceleration.

## Validation ladder

1. `tools/check_math.py`: compile the installed C model at O0 and O3 with strict warnings and, outside Windows, undefined-behavior sanitizer. No Docker or emulator needed.
2. Repository host tests: independent integer/oracle checks for random and extreme arithmetic, all 1024 trig angles, roots, normalization, interpolation monotonicity, asset regeneration and generator refusal to overwrite.
3. Repository `tests/math_cpu.py`: execute actual compiled helpers on Unicorn's **original M68000** model, including SDK word multiply, checked DIVU, dot overflow, in-place results, matching logical PAL/NTSC time, pause/catch-up and VBlank publication. MMIO writes are captured; this is not an emulated VDP or cycle benchmark.
4. ARM64/AMD64 builds: US/JP/EU releases, US debug and deterministic tour. Reports must contain executed math/publication evidence and identical target payloads; omitted regional/tour evidence fails.
5. ares: boot each regional disc with its configured BIOS, inspect all three views, target movement, reset, pause and sound. Launch explicitly with `--system 'Mega CD'`. Never distribute a BIOS.
6. Real hardware still required for bus/interrupt timing, upload headroom, audio and device/region behavior. Do not infer hardware performance from an instruction count or a smooth host animation.

Record toolchain revision and emulator/hardware evidence with results. Cross-architecture equality is reproducibility evidence; it cannot prove a shared arithmetic bug is absent, hence the independent oracles.

# Validation and measurement

## Useful behavior tests

Test the real failure policies, not just a particle counter:

- Fill impact capacity and request another burst. Confirm rejection is bounded
  and weather cannot consume the impact reserve.
- Compare one second of PAL and NTSC updates, including emission and PRNG state.
  Pause must freeze lifetimes/randomness; an edge action must occur only once.
- Exercise every lifetime through retirement/reuse, clamped catch-up and all
  weather/density choices. Simulation continues for visually rejected particles.
- Force scanline overload and full SAT capacity. Check links, priority order,
  per-line entry/pixel limits, culling and a cleared empty head.
- Verify shake leaves model/collision coordinates unchanged and returns exactly
  to zero. Check the camera borders, HUD stability and the disable control.
- Mutate the model while a snapshot is pending: it must not change the published
  frame. Reject invalid indices/counts; late VBlank defers publication.

`tests/test_effects.py` compiles the portable model at O3, runs a combined
10,000-update stress test at O0/O3 (UBSan where supported), validates native art
and checks standalone creation without overwriting projects. Test dimensions
and limits describe the Lab; they are not a promise about arbitrary game art.

## Actual target execution

`tests/effects_cpu.py` runs the compiled original-68000 functions, including
fixed-point movement/gravity, lifetimes, pool rejection, timing equivalence,
scanline admission and immutable publication. It executes the VBlank consumer
and compares every SAT, scroll and HUD data word with its snapshot. It verifies
ordinary frames do not write CRAM. This is executable behavioral evidence,
not cycle-accurate timing or physical-console evidence.

Build US/JP/EU release, US debug and the automatic tour on native ARM64 and
AMD64 CI. Compare target payloads and require executed effects/publication
markers in both reports. Install the standalone skill in a disposable project
with telemetry disabled in CI.

## Ares audition

Load the CUE explicitly as **Mega CD**, using a personally supplied BIOS:

1. Move the lantern and look for the fading trail.
2. A bursts into sparks and rising smoke; holding A should not continuously fire.
3. B cycles clear/rain/snow. C cycles four weather densities.
4. At maximum snow, keep moving and repeatedly request bursts. Check the FPS
   counter, important-effect visibility and deliberate SKIP count under crowding.
5. Start pauses effects; resume should not jump to catch up. Up+Start toggles
   shake without moving gameplay coordinates. The HUD must remain still.
6. Repeat in PAL. Simulation speed and lifetimes should match, while published
   frame rate normally reads 50 rather than NTSC's 60.

The displayed FPS counts consumed frame snapshots per video-clock interval,
not particle updates and not the emulator's host VPS. A stable counter does
not establish CPU headroom. The automatic tour runs bounded model checks and
then continuously combines maximum snow with periodic bursts. Manual controls
and presentation are separate checks.

Inspect simulation/list construction and VBlank timing with the measuring HUD
both on/off if results are close to budget. The Lab's four HUD rows are a known
fixed cost, not free instrumentation. Check worst-case spatial concentration,
not just a uniform field. Never infer cycle utilization from a byte count.

## Hardware still required

Real hardware must qualify VBlank completion, sprite-fetch quirks, worst-case
bus contention, PAL/NTSC variants and the full game's audio/CD/ASIC workload.
Ares observations and Unicorn execution cannot certify these. Adding effects
must preserve the game's existing scheduling and resource owners; simultaneous
work can change costs even if this standalone Lab stays smooth.

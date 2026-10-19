# Validation and evidence

## Executable behavior

The standalone `tools/check_animation.py` compiles/runs timeline and controller
fixtures at O0 and O3, with undefined-behavior sanitizer on supported hosts.
They check:

- clip bounds, zero/oversized holds, invalid event/pose/weapon/box fields;
- same-clip selection without phase reset, named table validity and restart;
- every loop footstep, first-frame events and completion emitted once;
- terminal pose retention, explicit revival and interrupted active windows;
- mirrored hitboxes, half-open overlap and arithmetic near signed bounds;
- repeated contacts versus one hit per action, including crossed poses;
- held/queued input, captured modifiers, pause and bounded catch-up;
- equal-time PAL/NTSC state and quarter-speed behavior.

Art tests regenerate source bytes at native resolution, check legal VDP words
and RGB/BGR channel packing, and verify exact debug-outline dimensions.
Reused graphics and IRQ bridge files have identity checks. These checks do not
measure console performance or substitute for viewing the art.

## Target execution and build matrix

Repository integration builds US/JP NTSC, EU PAL, debug and tour variants on
ARM64/AMD64. Native CI runners compare complete payloads and require explicit
animation execution evidence. Local AMD64 on an ARM Mac is emulated Docker, not
a native AMD64 host. Preserve original-68000/compiler safeguards and linker
capacity rejection; do not weaken optimization defaults to hide a behavior bug.

Original-68000 execution checks actual model/controller instructions, equal-time
state, one-hit behavior, interruption, terminal death, revival, pause and invalid
selection. The compiled tour uses those same transitions. The real publisher is
checked for immutable snapshots, deferred late service, exact SAT/HUD writes,
invalid-count refusal and no normal-frame CRAM writes. Captured MMIO is not a
VDP emulator or evidence that arbitrary interrupt timing is safe.

## ares review

Launch with `--system 'Mega CD'` and the generated CUE, using the user's configured
legal BIOS. Test both PAL and NTSC:

1. Walk Left/Right. A loop should keep its phase, with footsteps tied to poses.
2. Approach the dummy and press A. It should wobble, with one HITS increment per
   swing even when several active poses touch it. Hold A to check edge semantics.
3. Press B during windup/active/recovery. Hurt removes the sword/hitbox immediately
   and the abandoned attack does not later complete or hit.
4. Hold Down and press B. Let the fall reach its last pose; only then A rises.
5. Use Up for quarter speed, C for actual hitbox outlines and Start for pause.
   Pause must stop poses, events and hit counters while UI remains responsive.
6. Check native art, readable/stable HUD, finite sound cues and no CRAM dots.

Tour mode attacks once, interrupts another windup, falls, retains the final pose
and rises. AUTO PASS is a repeatable smoke result, not exhaustive gameplay or a
frame-rate benchmark. The art, dummy, combat rules, timing and controls are
illustrative and should not influence an original game's style or balance.

Record emulator acceptance only after observation. Physical raster/FIFO timing,
controller/BIOS variants, sustained mixed loading/audio workloads, stack maxima
and actual hardware frame cadence remain separate qualification checks.

# Bounded Megadev integration

## Ownership seams

Keep damage resolution on the same simulation snapshot as movement and attack
poses. Rendering consumes the committed result; audio consumes event bits. An
existing game's service owners stay installed. The Lab uses one Main VBlank
handler, saved-register BIOS adapters, `BIOS_TRIGGER_IFL2`, `BIOS_READ_JOYPAD` and
`bios_joy1_hold/hit`, `bios_joy2_hold/hit` from `main/bios.h`. Both pads are read
through that single owner. Device protocols and multitaps belong to `megadev-input`.

The verified interfaces come from [Megadev 7a7246c](https://github.com/drojaazu/megadev/tree/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib).
Reinspect the selected container before changing assembly conventions. No SGDK
sprite engine, `fix16`, task facility or ResComp format is assumed.

The Sub bootstrap loads Main and remains available for BIOS INT2/CD service.
Combat does not borrow Word RAM or presume the Sub is a free physics processor.
If profiling justifies offloading candidate generation, transfer immutable input
and tagged results through the existing `megadev-system` protocol. Main validates
scene/actor generations before committing results. Never share live actor pointers.

## CPU and RAM bounds

The illustrative model has five actor slots, eight bolt slots and no heap or
recursion. A sword can test only opposing actors (at most 12 pairs across both
teams); eight bolts test at most three opposing actors each. The conservative
52-candidate test ceiling includes slack. Broad-phase grids are unnecessary for
this size; measure larger workloads before adding one. Cap catch-up at four
60-Hz steps per foreground call. Pending input edges survive partial steps.
PAL contributes six 300-Hz units per video frame; NTSC contributes five.
Pause clears pending edges and freezes attacks, cooldowns and movement.

Knockback passes through axis-separated movement bounds. It cannot push through
arena plinths. This is not a replacement collision engine: use the game's swept
solver for faster motion, larger impulses, moving platforms or irregular geometry.
Bolts move four pixels/step, so the sample's minimum solid extent/hurtbox and
six-pixel projectile permit discrete tests. Increasing speed requires a sweep.

## VDP and audio

Resident native art uses four normal palettes, opaque 5×7 font glyphs in 8×8 cells,
and column-major sprite tiles. Patterns, map and palettes load with display off.
Foreground code stages two immutable SAT/HUD frames. The VBlank callback checks
index, count and early VBlank before uploading; late work stays pending. Opaque
end/help panels update only on state changes. Sprite priority falls below the
panel while it is displayed. Normal updates never write CRAM. H40 scanline
admission remains the graphics owner's job, including sword/guard effects.
The Lab admits bolts, bodies and weapons before optional hit flashes/HP pips.
Their worst coincident H40 row is 18 sprites and 304 pixels, within 20/320;
the native publisher test exercises that row. Extra combat entities or wider
weapons need a new budget; never silently hide a collidable projectile.

One Lab-only PSG owner plays finite cues and a finite victory fanfare. Its simple
music is illustrative. Route events into the game's existing CD/PCM/FM/PSG policy
instead of writing sound hardware alongside another driver. Coalesced event bits
are sufficient for this example; games needing every distinct event should use
a finite queue with an explicit overflow/priority policy.

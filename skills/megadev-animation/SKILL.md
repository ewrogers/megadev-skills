---
name: megadev-animation
description: Build gameplay-driven sprite animation in Megadev Sega CD/Mega CD games with named clips, frame holds, transitions, timed events and hitboxes. Use for playback/gameplay synchronization and interruption rules, rather than asset generation, a general combat system or VDP rendering APIs.
---

# Megadev animation

Start with the game's existing simulation clock, action state and renderer. Megadev does not supply SGDK's sprite animation API. The Lab's `an_*` and `tr_*` routines are application code, not SDK functions.

## Author a timeline, not a drawing-side timer

Use [clips and timing](references/clips-and-timing.md) for named clips, per-frame holds, loop/terminal behavior and completion semantics. Advance playback from the existing game steps. A renderer may skip intermediate poses; it must not decide which gameplay events happened.

Keep logical frames separate from stored graphics. Several frames can reference one pose with different holds or events, without duplicating image patterns. Check frame ranges and duration bounds before selecting a clip. Avoid restarting the current walk/idle clip on every update.

## Make transition rules explicit

Use [events and interruption](references/events-and-interruption.md) for event identity, state priority, cancellation, terminal poses and one-hit contracts. Consume entry/completion events during the simulation step where they occur. If events are queued, carry the originating action identity and discard stale work after interruption.

A sword pose and its active hitbox should describe the same reach. Test collision on every simulation step, then apply per-action/per-target damage policy. Invulnerability, guards, combos and balance belong to gameplay/combat rules; an animation frame alone does not authorize damage.

## Integrate through existing owners

Read [Megadev integration](references/megadev-integration.md) before adding playback to a project. Reuse its clock, input, VDP publication, audio and residency policies. Animation does not need one timer, allocation or Sub job per actor. Additional CPUs and disc space do not remove VRAM, sprite or upload limits.

`megadev-gameplay` owns the simulation loop and movement. `megadev-graphics` owns rendering and sprite admission. `megadev-assets` owns preparation; `megadev-audio` owns sound requests; `megadev-scenes`/`megadev-streaming` own residency and retirement. This skill connects those contracts to animation without installing competing services.

## Prove the behavior

[Animation Lab](assets/animation-lab/README.md) demonstrates a duelist and practice dummy, visible hitboxes, slow motion, hurt interruption, death and recovery. The art, sound, controls, combat rules and tuning are illustrative, not a template for the user's game.

Use [validation](references/validation.md) for executable checks, original-68000 execution and PAL/NTSC ares review. Distinguish model behavior, compiled instructions, emulator presentation and physical-console timing. Increase animation/actor counts only after measuring the game's combined workload.

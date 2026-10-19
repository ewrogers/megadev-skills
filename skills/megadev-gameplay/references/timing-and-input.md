# Timing and input

## One game clock

The standalone Lab installs one Main VBlank path and samples the BIOS pad there.
It increments an unsigned 16-bit display counter even when there is no new image.
Foreground computes an unsigned delta, converts each NTSC frame to 5 and each PAL
frame to 6 units of a 300 Hz clock, then advances a 60 Hz simulation every 5 units.
Thus 60 NTSC frames and 50 PAL frames produce 60 simulation steps. PAL still
presents only 50 images per second; the Lab does not interpolate between poses.

A quarter-pixel position and 6 quarter-pixels per cardinal step produce 90 pixels
per second. Diagonals use 4 per axis, approximately the same distance. This is
illustrative tuning, not a required movement feel. Choose fixed-point range from
world size and intermediate arithmetic, not just visible screen dimensions.

`gp_advance(state, elapsed300, held, pressed)` retains a fractional remainder.
It caps one call at 20 units/four steps and records each clamped call in `dropped`.
Excess time is discarded, with no growing backlog. A long stall therefore slows
game time; it does not simulate arbitrary missed history. An existing game may
prefer another bounded policy. Measure it and document what replay records.

## Held state and edges

Megadev's Main BIOS input uses SACBRLDU: Up=1, Down=2, Left=4, Right=8,
B=16, C=32, A=64, Start=128. Use `<main/io.def.h>` names at the adapter boundary.
Do not import SGDK button numbers.

The Lab ORs `bios_joy1_hit` into a word in its IRQ and snapshots/clears it with
interrupts masked. Its latest `bios_joy1_hold` sample applies across any catch-up
steps. A pending A edge survives a call with no full simulation step, then is
consumed once by the first step. Holding A does not repeatedly pulse.

This is a bit latch, not a timed input recorder: repeated presses within a stalled
foreground interval can collapse. Precise replays should record input per logical
step and include any timing/overload policy, following the existing testing owner.
Opposing directions cancel rather than giving hidden priority to one button.

## Pause and deadlines

Start toggles pause and clears pending gameplay input/remainder. Paused calls do
not advance movement, enemy stun, invulnerability, effects or pulse recharge.
Resume does not catch up the time spent paused. Pause is a UI action and is
handled even when no simulation step is due.

Main/Sub service observation keeps running from elapsed display time, independent
of the game clock. Do not pause a cancellation drain, CD access loop, audio owner
or resource lease just because actors stop. The Lab caps one service observation
at its one-second timeout; this is not an accurate wall clock after an unobserved
multi-second stall or a 16-bit counter wrap. Keep the foreground responsive and
use the platform service owner's deadline policy in a real game.

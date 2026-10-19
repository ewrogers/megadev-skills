# Clips and timing

## Data model

The Lab uses immutable C tables of named clips (`an_clips`) and stable enum IDs.
Names support authoring/inspection; hot playback selects an ID, not a string
search. Its `AnFrame` contains a body pose, hold, entry-event bits, optional
weapon pose and optional hitbox. This is application data, not a Megadev asset
format or SDK animation object.

A frame hold is a positive number of **game simulation steps**, not VBlanks.
The sample supports 1..8 frames per clip and holds 1..600 steps. These small bounds
are illustrative. Several frames may use the same stored body pose, so a long
idle hold or timed event does not require repeated graphic patterns.

Zero-initialize `AnPlayer` before its first `an_play`. `an_play` validates a new
selection and reports `AN_STARTED` plus any first-frame events. Selecting the
same clip without explicit restart leaves its phase intact, including a finished
terminal pose. An invalid selection preserves the old player.

`an_step` consumes exactly one simulation step. On a boundary it returns the
new frame's entry events. Looping clips return to frame zero without completion.
Non-looping clips emit `AN_COMPLETE` once after the final hold, retain that pose
and stop advancing. Repeated updates do not repeatedly complete. An explicit
restart creates a new action generation and re-enters its first frame.

The validator assumes valid, readable C pointers; it is not a binary-file parser
or a safeguard for arbitrary memory. Imported clips need a bounded loader and
asset/range validation before becoming runtime tables.

## Clock ownership

Animation core has no elapsed-time accumulator, hardware timer or IRQ hook.
An existing game's fixed-step loop calls playback once for each logical step,
processing events and collisions before the next step. Do not only advance to
the latest visible pose, then inspect that one pose for all skipped history.

The standalone Lab has one clock, following `megadev-gameplay`'s 300 Hz elapsed /
60 Hz simulation contract: 5 units per NTSC VBlank, 6 per PAL VBlank, 5 per game
step. It owns that loop only within its own disc; it does not run a second clock
alongside an existing game. PAL presents 50 images while still simulating 60
steps per second. No visual interpolation is claimed.

At most four steps run per foreground call; excess elapsed time is discarded and
a clamped-call counter increments. Pause clears pending input/remainder and stops
poses/events. Quarter-speed inspection divides the Lab's elapsed time by four
before simulation, retaining fractional time. It slows movement, playback and
its small Lab sound track together; it is not a required game's pause/audio policy.

Define overload behavior before recording replays. If an existing game uses a
different clock or time scale, adapt frame holds to it instead of importing this
sample's assumptions or installing another timer service.

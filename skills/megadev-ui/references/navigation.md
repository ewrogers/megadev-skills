# Navigation without input surprises

Consume the input owner's coherent snapshot. Megadev's BIOS pad state suffices
for this pad example; it does not replace the mouse/multitap/device contracts in
`megadev-input`. Map application actions through the game's input system.

Use pressed edges for accept, cancel, pause and purchases. Holding Accept must
not repeatedly activate pages. Directional repeat is separate: move once, wait,
then repeat at a controlled rate. Cancel opposing directions and decide diagonal
arbitration explicitly. Do not replay a whole missed backlog after a long load.

Menu Lab uses 300 Hz integer units: five per 60 Hz frame, six per 50 Hz frame.
Repeat starts after 90 ticks, then every 24. Normal repeats retain fractional
overshoot; a stall emits at most one event. The platform adapter caps missed
elapsed frames. These are equal-time logic checks, not timing measurements.
Reuse another game's existing clock rather than installing this convention.

Keep focus per page. Back from Details selects the item you opened. A bounded
history stack stores the return route, not duplicate renderer resources. Refuse
an overfull stack before changing state; validate focus/history before indexing.
Disabled or empty cells need an explicit navigation rule and visible state.

Pause game simulation independently from menu clocks and input polling. In this
example Start resumes from Pause; Back from a nested screen restores that menu's
focus. Test Options from both title and pause. Settings are in-memory only.

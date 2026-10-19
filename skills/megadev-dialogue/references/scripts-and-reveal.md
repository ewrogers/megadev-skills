# Scripts, reveal and branches

Keep content separate from mutable conversation state. A useful script record
names a speaker, bounded spans, optional voice, choices and destination IDs. A
game may author this in another format and compile it offline. Do not make the
example's C arrays a mandatory authoring format or invent a Megadev script API.

## Bounded text

The Lab's `DSpan` stores a pointer plus explicit byte length, palette, font face
and a pause before the next visible glyph. `d_compile` supports up to eight spans
and 512 source bytes. It creates at most four 22×3 pages, rejects overflow without
changing the output, and stores each cell's glyph, style and reveal delay. Newline
moves a row; form feed starts a page. Whole words wrap when they fit the row;
long words hard-wrap. Keep style boundaries at word boundaries for this simple
per-span word lookahead. Empty intermediate pages are rejected.

Glyphs are a bounded single-byte Latin demonstration. Out-of-range bytes use `?`;
the prepared art maps lowercase letters to uppercase-looking glyphs. This is not
UTF-8 shaping, proportional layout or full localization. Match actual glyphs,
text lengths and translated layouts using the project's text/localization owner.

The compiler does not resize glyphs or silently truncate a conversation. Inspect
native previews, longest lines, explicit page breaks and every choice label.
The two faces and palette style on “Listen” are examples, not required styling.

## Time and controls

Use the game's elapsed-time convention. This example uses 300 Hz units: five per
60 Hz frame and six per 50 Hz frame. A normal glyph costs nine ticks, a comma adds
30 and a sentence-ending mark adds 60. Spans can add an explicit bounded pause.
Blank padding has zero delay; reveal work still examines at most 66 cells per call.
Elapsed time is capped after a stall, rather than replaying an unlimited backlog.

A to reveal fills only the current page and stops its voice. A later pressed edge
advances the page or commits the selected choice. Holding A cannot accept several
lines. Choices become active only after the last page is readable. Directional
repeat is bounded and cancels opposing directions. A choice commits once; branch
state is independent from its visual highlight.

B returns to stories and cancels voice intent. Start pauses reveal and stops the
current excerpt; resume continues subtitles without replaying that excerpt.
C toggles voice; turning it back on applies to a later voiced line. Different
games may need replay, audio pause/resume or another accessibility policy.

Rendering a new page takes time. `d_present` accepts only its current generation;
no glyph timing or voice starts before that acknowledgement. Old completions do
not advance a newer page. Keep this handoff when replacing the rendering backend.

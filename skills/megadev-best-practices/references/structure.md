# Structure and review

Keep boundaries around responsibilities and invariants, rather than forcing one
folder arrangement on every game. A small game can use a few files. A larger one
benefits from private service state and interfaces that expose requests/results
without requiring callers to understand every register or BIOS detail.

Harbor Relay illustrates three useful parts:

- Portable policy/records (`scene.c`, `audio.c`, `stream.c`, `save.c`) can be tested
  without inventing a second hardware implementation.
- Platform adapters own the Main/Sub register, BIOS and publication rules.
- Foreground glue makes game decisions and composes existing services.

`checkpoint.h` is the example's application-facing seam. Its caller supplies a
snapshot and consumes a completed result; it does not write BIOS scratch pointers
or audio mailboxes. The small contract helpers are examples, not a new common
utility package that every game must adopt.

For a change or review, ask concrete questions:

- Which code owns each allocation, cached pointer and in-flight buffer? What
  event makes reuse safe, and can cancellation/timeout occur before that event?
- Is an interrupt consuming a complete immutable snapshot? Is the producer
  waiting for acknowledgement before writing it again? What does a missed frame do?
- Are byte counts, word counts, sector counts and CPU addresses distinguished?
  Can a sum overflow before the bounds check or an odd pointer reach a word load?
- Are save, audio and load results checked before changing visible game state?
  Can a late completion act on a different scene or newer user choice?
- Does the actual original-68000 build link with the intended flags, RAM layout
  and stack reservation? What does its target disassembly say about a hot operation?

Do not add a generic allocator, scheduler, event bus or second renderer just to
join two systems. If a shared owner needs a new capability, extend and test that
owner's contract. If an integration bug affects existing skills, fix the owning
implementation and all affected examples instead of preserving divergent copies.

Record ownership assumptions next to the fragile boundary and in a short project
resource map. Keep comments about why a handshake/order is necessary; avoid long
narration of each assignment. User-selected architecture and gameplay take priority
over the Lab's naming, clocks, button choices and progress format.

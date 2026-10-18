# Find the first broken contract

1. Keep a known-good build and a short reproduction: scene, region, device setup,
   inputs, timing and expected result. Record whether the symptom survives a
   fresh boot and release build. Change one variable at a time.
2. Observe a Main heartbeat, Sub heartbeat and the current request phase. A
   changing Main tick with a reserved buffer points to a service wait; neither
   moving suggests a different problem. A counter alone does not prove useful work.
3. Capture the request token, committed reply, result, fresh memory mode and
   progress. Keep metadata before the commit word and do not rearrange the
   established ownership protocol to make the display easier to write.
4. Validate counts/offsets before access. Guards catch selected boundary mistakes,
   not every out-of-bounds write. Never deliberately corrupt arbitrary RAM in a
   test; Debug Lab damages only its own prepared guard before granting the buffer.
5. Freeze the first anomalous transition. Continue safe draining and retirement.
   If the peer never returns, keep the resource reserved. Do not poke ownership
   bits, reset the Sub CPU or abandon BIOS/CD/audio state as a speculative fix.
6. Add a regression for the actual ordering or bounds error. Retest the optimized
   build, both regions, both build architectures, and affected service combinations.

## Trace cost and concurrency

`trace.c` uses sixteen 12-byte records, a 12-byte first-fault snapshot and eight
bytes of metadata: **212 bytes per CPU**. The ring overwrites oldest records,
counts overwrites with saturation, and freezes on the first fault. Clearing drops
visibility of old records; it does not scrub their bytes. A stale `first` record
is invalid whenever `frozen == 0`.

Both Lab traces have one foreground writer. The Main UI reads its own trace in
foreground; Sub's trace stays private in Program RAM under the `sub_trace` ELF
symbol. This is not an on-disc log, a cross-CPU live trace viewer or an atomic
multiwriter facility. Export a Sub snapshot only through the game's existing
owned transfer/commit protocol, or inspect it using a qualified debugger.

Do not add an ISR writer to this implementation. If needed, use a separate
single-producer queue, a bounded critical section with an explicit budget, or a
sequence-tagged snapshot protocol. `volatile` and aligned word stores do not make
an entire record atomic. Save/restore the prior interrupt mask in reusable code;
the Lab's mask/unmask calls assume its own foreground context.

Local 16-bit ticks wrap and clocks are not synchronized across CPUs. Tokens
correlate a transaction, not an absolute timestamp. Capture module identity and
build identity outside the hot ring. The Lab only records state transitions and
replies, never per-word transforms; VBlank publishes changed text cells and never
formats records or writes CRAM during normal display.

When investigating timing, first disable the trace UI, then the recorder, and
compare. Stopping one CPU in a debugger can leave the other running and alter
mailboxes, CD service and audio behavior. A stopped-context observation is not a
hardware timing measurement.

# Handoffs, cancellation and recovery

## A local deadline is not a remote stop

System Lab uses 300 clock units per second: 5 per NTSC frame or 6 per PAL frame. Its job deadline is one second. Main measures elapsed VBlanks, not how many times it redrew the UI. A response first observed at or after that deadline is discarded even if the worker reported success.

A timeout changes the client to DRAIN and requests cooperative cancellation using the current token. It keeps the slot and buffer reserved. A two-second artificial delay models a service that cannot acknowledge cancellation immediately; it is not a disc latency measurement.

When the matching late reply arrives **and** Main owns the buffer, Main discards it, acknowledges it and waits for the reply commit to clear. Only then can another request start. Pressing C uses the same drain path for a committed job, including the race where output finished just before cancellation. It does not magically interrupt a running BIOS call.

## Ownership confirmation can also fail

A grant request is not permission to access the other CPU's memory view. If the initial grant is not confirmed by its deadline, this example enters STUCK and refuses further work. It does not assume the buffer is reusable merely because no command was committed. A pending hardware handoff is a separate state from an idle software mailbox.

If the Sub CPU never returns ownership, a completed reply is missing, or retirement never clears, leave this resource isolated. The demonstration keeps its Main-side display/controls running and requires an explicit disc reset for an unrecoverable link. A production game can disable optional work or offer a higher-level recovery action using its established platform-service policy. Do not hard-reset the Sub CPU or reinitialize the drive as a generic timeout response.

## Make cancellation safe at a boundary

Check the cancellation token before the next bounded slice, after confirming ownership. Stop modifying output, return the buffer and report a distinct cancellation result. A cancellation may race with completion; the caller's discard flag determines whether that result is still wanted.

A rejected command also returns ownership before reporting completion. Unknown opcodes and out-of-range counts must not become arbitrary function pointers, memory writes or silently clamped requests.

## Scope and budgets

One job is outstanding in the Lab; attempts to start another are rejected. A chunk is at most 16 data words, but this is a work-count bound, not a measured CPU-time guarantee. Choose limits from the game's service deadlines and measure on hardware.

The Sub CPU also services BIOS/CD interrupts and may later own PCM/CD audio work. Keep interrupts enabled during ordinary worker slices. An allocator, queue, loader or audio service must participate in the same ownership contract rather than seizing Word RAM independently. 1M double buffering and complete memory accounting belong to memory/streaming guidance.

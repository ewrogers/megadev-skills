# Tests and qualification

## Behavior and target code

Host C fixtures run at O0/O3 with undefined-behavior checking where supported. They cover seed wraparound, exact chunk bounds and untouched future words, guards/checksums, invalid header/count/opcode, corrupt input, cancellation at every chunk boundary and invalid-owner paths that cannot dereference a buffer. Client cases include overlap rejection, stale replies, incomplete ownership, retirement, timeout saturation, PAL/NTSC elapsed time and replies first observed after a long foreground stall.

The repository integration helper builds US/JP/EU release, US debug and a US automatic tour on ARM64/AMD64. It executes the actual compiled Main and Sub adapters with shared modeled mailboxes and Word RAM. Scheduling varies observation order, skips worker slices and repeats a tick. Tests assert writer direction, metadata/result publication before commit, ownership before each buffer access, normal/rejected/cancelled/late results, cancellation just after completion and refused reuse after a failed grant.

The emulator harness models register visibility and instantaneous configurable ownership grants. The actual IRQ save/restore, VBlank clock and retained input edges execute with BIOS entry points stubbed. The harness stops at RTE because its backend cannot execute that return; ares checks the complete return path. It does not simulate Gate Array electrical timing, arbitrary instruction-level races, BIOS calls, full dual-CPU interrupts or a physical drive. Startup/INT2 continuation checks are separate. CPU execution of an instruction is stronger than a host model, but it is not hardware qualification.

## Try in ares

Use `ares --system "Mega CD" system_lab.cue` with the user's legal configured BIOS. Keep the disc immutable while loaded. The automatic tour is `TOUR=1`; it should end at AUTO PASS with 2 accepted, 1 rejected, 2 discarded and 0 bad-data jobs, while the Main light and clocks keep updating.

In the normal build:
- Move the Main light with the D-pad while A starts a valid job.
- B starts a deliberately delayed job: observe DRAINING, safe return and another A request afterward.
- C cancels a committed job; try it during B's delay too.
- Start cycles valid request, unknown command and invalid size. A should reject the last two cleanly and return the buffer.

A/B/C/Start are physical pad labels, not PC keyboard names. Preserve emulator bindings. Scripted tours prove runtime flow independently of unreliable automated key injection; they do not prove live pad mapping. Check PAL behavior separately from merely building an EU disc.

## Still needs real hardware

Verify other BIOS/models, actual handoff latency and mode transitions, interrupt deadlines, PAL/NTSC display behavior and coexistence with drive access, Z80 audio and Sega CD PCM/CD-DA. This Lab has no active audio service and no runtime file reads after bootstrap. Do not claim background disc streaming or sound compatibility from its arithmetic worker.

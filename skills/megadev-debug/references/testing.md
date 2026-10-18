# Qualification

- Host C at O0/O3 (UBSan where available): ring wrap/order, invalid index, saturated
  overwrite count, frozen snapshot stability, tick wrap and explicit clear.
- Paired compiled original-68000 execution: healthy and invalid requests, transient
  stale replies, cancellation, timeout/drain, guard corruption before handoff,
  no unowned buffer access, no premature reuse, preserved Main/Sub first faults,
  safe later jobs, stalled grant, callback registers and input edges.
- Actual linked ELF lookups inside the toolchain: Main, Sub and Room code;
  `-g` source attribution, wrong-CPU rejection. Host negatives additionally reject
  wrong hash, odd/outside PC, wrong module and unused text-window padding.
- Native ARM64/AMD64 CI builds US/JP NTSC, EU PAL, `-Og -g DEBUG=1` and automatic
  tour cases. Compare disc files, boot bytes and executed-result markers exactly.
- Standalone installation verifies all resources with telemetry disabled. The
  generator refuses existing destinations. `coord.c` remains byte-identical to
  the coordination skill, making that ownership boundary testable.

## Ares checklist

Boot `debug_lab.cue` with `--system 'Mega CD'` and a user-provided BIOS.
A runs the selected case; Left/Right changes it. Healthy increments Accepted;
bad command/size increments Rejected, and Buffer Guard freezes GUARD with detail
0005 (SY_BAD_DATA). C clears the Main trace only when idle. Clear before each test
if you want a new first-fault snapshot. B deliberately delays a healthy job:
TIMEOUT freezes while DRAINING, then Discarded increments and READY returns.
Start opens the last eight Main events. The Main/Sub ticks should keep advancing.
Try a healthy job after recovery; frozen evidence must remain until cleared.
Repeat with EU/PAL and the automatic tour. Check steady text and no CRAM dots.

Unicorn models mailbox and ownership semantics, not BIOS/drive latency or VDP
scan timing. ares is a functional check, not physical timing proof. Fatal handlers,
real console/BIOS variants, sustained CD/audio coexistence, interrupt timing and
external debugger transport remain separate qualification work. A missing peer
cannot be fixed by the ring recorder.

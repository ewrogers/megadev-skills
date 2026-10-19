# Qualification and limits

- Host C O0/O3: 1,200 phase/load combinations produce equivalent batch output;
  invalid parameters, twelve-bit wrap, ambiguous stamps, window wrap/expiry,
  stalled/oversized observations and timed/untimed accounting are checked.
- Original-68000 execution: independent Python output oracle, actual stopwatch
  reader with injected wrap/incoherence, timer-off behavior, no stopwatch writes,
  HUD-off behavior, warm-up and safe drain before buffer reuse.
- Actual compiled Main/Sub ASIC pipeline: original scaling regression checks
  trace/image oracle, grant/token ordering, immutable snapshots, overlap,
  two-chunk early-blank publication, stable HUD and non-returning jobs. No second
  ASIC/ownership implementation is introduced.
- Native ARM64/AMD64 CI: US/JP NTSC, EU PAL, explicit debug and automatic-tour
  builds. Complete payloads and executed-result markers must match. Installed
  resources and standalone helpers are tested with telemetry disabled.
- ares: repeat the controls/window comparisons on both rates; check readable
  counters, case switching, complete images, CHECK OK and responsive input.

Injected stopwatch values prove arithmetic and sampling policy, not processor
cycles or a real stopwatch implementation. Unicorn is not a Gate Array, VDP,
ASIC timing or CD/audio contention simulator. The Lab's numbers are observations
of the selected emulator/console configuration, not universal performance targets.

Physical console/BIOS variants, shared stopwatch resets, sustained drive/PCM load,
real VBlank/transfer timing and analog audio quality remain separate qualification.
When measuring on real hardware, keep a safe fallback and invalidate samples if
another clock owner or long interrupt mask violates the assumptions.

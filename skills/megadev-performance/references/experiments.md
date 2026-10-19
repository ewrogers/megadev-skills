# Controlled experiments

Keep case, load, inputs, method, HUD/timer settings, region/video target, build
flags and assets with every result. Repeat trials and retain ranges rather than
one attractive minimum. A result is useful only with its measurement boundary.

The Lab runs a fixed nominal four-second window, resets the CPU phase/request
inputs and warms the layout before starting. ASIC cases also finish one unmeasured
job first, excluding lazy source initialization. Final result checking runs after
timing stops. It uses one CPU batch per foreground wake, so the scheduler itself
limits completed work even when a batch has plenty of headroom.

- B changes remainder/incremental method with the same input/output contract.
- C changes the live counter HUD, which refreshes at roughly 10 Hz. It does not
  disable the common frame callback, sprite or pipeline renderer.
- Start disables/enables stopwatch reads; completed-work counting remains active.
  This is a timer-overhead comparison, not a completely uninstrumented game.
- A repeats a window. Configuration changes cancel measurement and safely drain
  old jobs before a new warm-up/window. Cancellation is not forceful hardware abort.

## PAL and NTSC

A nominal four seconds is 240 NTSC versus 200 PAL callbacks. Work per foreground
wake and the fixed 4 KiB-per-blank upload allowance therefore have different
per-second ceilings by design. That difference is not proof that the CPU or ASIC
itself runs proportionally slower. PAL also offers different display/blank timing;
this Lab deliberately keeps a conservative common transfer cap.

Use CPU batch samples for the same bounded kernel and throughput over actual
reported window duration for scheduling/pipeline questions. Confirm ares actually
runs the intended display rate. Do not compare an emulated PAL image executing at
an unintended rate against a nominal-label calculation.

## Recorded-run comparison

`tools/compare_runs.py a.json b.json --vary method` supports method, hud, timer or
region as the one declared axis. Each JSON object has exactly these fields:

- Integer `schema` (1), `case` (0 CPU, 1 upload, 2 ASIC, 3 combined), `level` (0–3),
  `method` (0 remainder, 1 incremental), `hz` (50/60).
- Boolean `hud`, `timer`, `check_ok`.
- Integer completed counters `elapsed`, `loops`, `late`, `blocks`, `upload_kib`,
  `images`, copied from the finished run. Values must fit unsigned sixteen bits.

The tool divides counts by `elapsed / hz`; a zero baseline produces a null ratio.
It rejects incomplete/failed runs and confounded settings. These inputs are
manually captured counters, not authenticated hardware traces. Save compiler/SDK,
disc/ELF hashes, emulator/hardware configuration and trial notes alongside them.

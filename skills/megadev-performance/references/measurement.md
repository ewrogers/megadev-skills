# Measurement contracts

## Stopwatch samples

The Lab brackets only its synchronous Main CPU kernel with **reads** of
`ga_reg_stopwatch & 4095`. It keeps interrupts enabled. The difference includes
preemption and bus stalls within that interval; it is not exclusive instruction
time, cycle count, whole-loop cost or CPU utilization. UI formatting, scheduling,
copy/transfer work and the reference recheck are outside this particular interval.

At the SDK's nominal 30.72 microseconds/count, the twelve-bit counter wraps in
about 125.8 ms. The example accepts a sample only when each stamp agrees with its
surrounding Main callback count and its span is at most three serviced VBlanks.
Under a functioning nominal 50/60 Hz frame clock this is shorter than one wrap.
The modular subtraction then handles a wrap correctly. If interrupts are masked
long enough to hide physical frames, or another owner resets the clock, this
qualification is insufficient: reject the experiment rather than trusting a
plausible result. No general reset detector is claimed.

REJECTED counts ambiguous timing samples. They do not erase completed-work counts.
A displayed `-----` means no accepted timing sample, or that the timer is disabled.
Zero ticks can mean below timer resolution, not free execution. READ PAIR shows
back-to-back register-read cost at this resolution; it is not a complete observer
cost estimate and is not subtracted to manufacture a precise cycle result.

## Window and pipeline counters

| Field | Meaning |
| --- | --- |
| MAIN LOOPS | Foreground batches observed in the window; not emulator VPS. |
| LATE BLANKS | Extra serviced VBlanks between foreground wakes (`delta - 1`); an observation of delays, not proof of every hardware deadline missed. |
| 64W BLOCKS | Completed CPU kernel words divided by 64. Compare only equal workload definitions. |
| UPLOAD KIB | Completed 1024-byte payload units passed through the bounded image-upload callback while measurement is enabled. Excludes map/HUD/SAT/command traffic, so it is not total VDP bus use. |
| IMAGES | Complete two-chunk images made visible during the window. |
| WINDOW BLANKS | Observed elapsed callbacks; the nominal four-second target can overshoot by the final batch. Normalize over this value. |
| JOB BLANKS | Last Main-observed interval from accepted ASIC request to handshake retirement. Includes service waits, source setup, copy/backpressure and scheduling; it is not raw ASIC time or Sub CPU utilization. |

A pending transfer/job can finish draining after measurement stops; it is not
credited to the completed window. The next case waits for safe retirement. Work
counts, timing fields and pipeline latency answer different questions; never add
them into a fabricated percentage.

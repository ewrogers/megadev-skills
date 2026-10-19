# Performance Lab

Repeatable Main CPU, VRAM upload, ASIC pipeline and combined workloads. It reports
completed work and elapsed timing with explicit boundaries, not an invented CPU
percentage. The native mosaic and small foreground sprite illustrate pipeline
progress; art, controls and workloads are examples rather than game requirements.

## Build

From the installed skill folder:

```sh
python3 scripts/create_project.py my_perf
cd my_perf
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest cue perf_lab.iso
ares --system "Mega CD" perf_lab.cue
```

Docker runs Linux containers on ARM64/AMD64. PowerShell uses `${PWD}` for the
absolute mount path. Supply your own BIOS; none is included. Use a fresh copy for
`build REGION=EU VIDEO=PAL`, `build TOUR=1` or explicit debug flags. TOUR runs all
four workloads once. Main/Sub boot and the unused Room fixture retain the existing
Megadev startup contracts.

## Controls

| Pad | Action |
| --- | --- |
| Left / Right | Main CPU / VRAM Upload / ASIC Pipeline / CPU + ASIC + VRAM |
| Up / Down | Four CPU load tiers; only CPU/combined cases perform this work |
| A | Repeat a nominal four-second window |
| B | Remainder / equivalent incremental wrapping method |
| C | Live counter HUD on/off |
| Start | Stopwatch sampling on/off |

Changing a setting safely drains old work, warms the display and, for ASIC cases,
finishes an unmeasured warm-up job before restarting. With HUD off, the initial
MEASURING snapshot stays still until results appear. The common renderer and
foreground sprite still run. The HUD normally refreshes around ten times/sec.

## Read the results

All numbers are decimal. MAIN LOOPS is foreground batches, LATE BLANKS counts extra
serviced callbacks between wakes, and 64W BLOCKS counts CPU work. UPLOAD KIB counts
image pattern payload only; IMAGES counts fully visible two-chunk images. Neither
includes all map/HUD/SAT traffic. WINDOW BLANKS is the actual reported interval,
which can slightly overshoot the nominal four seconds.

WORK TICKS / MIN / MAX bracket the Main kernel only, with interrupts enabled.
One tick is nominally 30.72 microseconds per SDK documentation. `-----` means no
accepted sample or disabled timing. READ PAIR is two clock reads at that resolution;
zero means below resolution, not zero overhead. REJECTED counts ambiguous stamps.
The shared stopwatch is never reset. JOB BLANKS is the last Main-observed request
latency, including waits/copy/retirement; it is not raw ASIC execution time.

CHECK compares the last CPU batch against the reference **after** measurement.
The host/CPU tests cover wider equivalence. Timer-off runs retain work accounting.
Try the same case/load with one setting changed at a time and compare several runs.

One batch per foreground wake and 4 KiB per early VBlank are deliberate policies.
PAL has fewer wakes/uploads per nominal second; this is not a direct CPU-clock or
maximum-bandwidth comparison. The combined case does not include CD/audio loads.
No physical timing or hardware certification is claimed by emulator/model tests.

## Standalone checks

`python3 tools/check_work.py` needs a host C17 compiler and prints correctness JSON;
it is not a console speed benchmark. `python3 tools/compare_runs.py a.json b.json
--vary method` normalizes manually recorded counters over `elapsed / hz` and
rejects failed/incomplete or confounded comparisons. See the skill's experiment
reference for the exact JSON fields and measurement limits.

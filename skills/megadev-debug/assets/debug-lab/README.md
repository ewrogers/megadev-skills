# Debug Lab — Signal Room

A small Megadev disc that preserves the first bad transition while Main/Sub
work safely finishes. Its two private 212-byte traces record state changes;
only Main's trace is shown on screen. The reusable coordination model is unchanged.
The panels and controls are examples, not a game framework or prescribed style.

## Build

From the installed skill folder:

```sh
python3 scripts/create_project.py my_debug
cd my_debug
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest cue debug_lab.iso
ares --system "Mega CD" debug_lab.cue
```

Use an absolute mount source; PowerShell uses `${PWD}`. Docker runs Linux
containers on ARM64/AMD64. Supply your own BIOS; none is distributed. Use a fresh
copy for `build REGION=EU VIDEO=PAL`, `build TOUR=1` or a source-debug build with
`build DEBUG=1 'OPT_FLAGS=-Og -g -fconserve-stack -fomit-frame-pointer'`.

## Try it

| Control | Action |
| --- | --- |
| Left / Right | Healthy, Bad Command, Invalid Size, Buffer Guard |
| A | Run selected case |
| B | Run a healthy job with an artificial delay |
| C | Clear Main's trace, only when the job is READY |
| Start | Summary / recent Main trace |
| Up / Down | Move the foreground control light |

Clear before trying another fault; otherwise the original evidence stays frozen.
Healthy increments Accepted. Bad Command/Size increments Rejected. Buffer Guard
damages this Lab's own prepared guard **before** ownership transfer, then safely
rejects it. The delay exceeds the deadline: TIMEOUT is captured, the buffer stays
reserved, the late result is discarded, and READY returns. This is not a crash.
Try a healthy job afterward. Main/Sub clocks should keep moving.

Trace columns and counters are hexadecimal. Events: START 1, PHASE 2, REPLY 3,
TIMEOUT 4, REJECT 5, GUARD 6, CANCEL 7, OWNER 8. Phase values match `coord.h`;
owner is the raw low three memory-mode bits. The last eight Main records are
visible; Sub records remain private under its ELF symbol `sub_trace`.

## Boundaries

The runtime owns one 2M Word RAM job, retains the established command/retirement
protocol and never forces ownership after a timeout. Runtime VDP writes go through
the existing bounded VBlank publisher; CRAM is initialized with display off.
The automatic tour checks accepted/rejected/cancelled/delayed work and reuse.
The Room module exists only for startup and symbol tests; the runtime does not
load it. No fatal exception vectors or emulator debugger connection are installed.

`tools/lookup_address.py` resolves an explicitly identified Main/Sub/Room PC
against the exact ELF hash; `tools/check_symbols.py . debug` qualifies all three
ELFs after a `-g` build. See the skill's symbol and testing references for limits.
Compiled CPU checks, ares and physical-console checks are distinct evidence.

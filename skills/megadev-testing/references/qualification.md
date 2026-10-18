# Region and evidence matrix

| Layer | Proves | Does not prove |
| --- | --- | --- |
| Host C at O0/O3 | Replay reset, validation, capacity, hashing, event consumption, first mismatch and equal-time clock arithmetic | 68000 ABI, BIOS, graphics or drive timing |
| Compiled original 68000 | Actual replay/coordinator code, startup, Main/Sub adapters, MMIO access/ownership and controlled completion ordering | A real Gate Array, VDP, drive, analog audio or cycle-accurate interrupts |
| Native ARM64/AMD64 builds | Both container toolchains generate the same target payloads and executed results | Console behavior just because host architectures match |
| ares | Boot, interactive controls, visible replay/fault behavior, actual emulated loading/music handoffs | All BIOS/models, real drive delays, physical controllers or analog quality |
| Physical console | Qualified setup's drive, service timing, display and audio behavior | Every region/model/peripheral without testing it |

The target matrix builds US/JP NTSC, EU PAL, explicit `-Og -g DEBUG=1`, and an
automatic tour. Keep original-68000 restrictions and the container's compatibility
defaults. The tour records a moving fixture, matches it, detects the deliberate
bug, matches again, then runs the live service suite.

## Equal elapsed time, not equal VBlank counts

The example uses 300 Hz elapsed units and one simulation step per five units:
NTSC contributes five units per VBlank, PAL six. Thus 240 NTSC or 200 PAL VBlanks
service the same 240 logical steps in four seconds. Movement, PRNG, gate event
and gameplay state must match. Presentation still occurs at the native display
rate; equal simulation does not mean equal screenshots or controller sampling.

The clock admits at most 30 elapsed units (six logical steps) per service call.
An overload is reported explicitly instead of silently dropping ticks. This is
an illustrative bounded catch-up policy, not a recommended universal 60 Hz loop.
A real game should choose its clock and overrun behavior for its CPU budget. Pause
policy must also be explicit: changing Lab pages suspends the course's logical
clock, while live CD/audio coordination keeps running.

Confirm the emulator actually runs each rate. Test the intended region and a
compatible user-provided BIOS. Also try start/stop recording, a full tape,
restarting after divergence and using slow uploads during music playback. Preserve
input mappings; automated key injection may fail even when physical input works.

Physical drive/BIOS variants, sustained CD/PCM contention, reset/power-loss behavior
and real controller combinations remain unqualified until tested. Never publish
emulator-only checks as a hardware certification.

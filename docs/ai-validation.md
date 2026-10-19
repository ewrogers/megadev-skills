# AI validation

Enemy Lab adds application-owned decision states and bounded navigation to the
existing Megadev rendering/input/bootstrap contracts. It keeps AI on Main CPU;
Sub offloading is documented as an optional measured design, not an implemented
SDK feature. The courtyard, enemy roles, music and game rules are illustrative.

## Behavioral evidence

Portable C17 O0/O3 contracts exercise facing/range before occlusion, corner
blocking, alert/pursuit/search/return, stun/recovery, input cooldowns, pause,
equal PAL/NTSC simulated time, unreachable goals and work caps. A busy search
cannot be replaced; map revisions and actor generations invalidate results.
An independent Python BFS checks shortest lengths and every returned route step
on 80 deterministic random maps. Tests are executable behavior, not matched
phrases in the skill.

The compiled original-M68000 check executes the full AI contract and actual
publisher. It checks Main BSS versus stack, immutable snapshots, delayed VBlank,
invalid sprite counts, exact SAT/HUD bytes, absence of CRAM writes during normal
publication, and clear/restart panel updates. A finite fanfare retires rather
than leaving a held completion tone.

The renderer reuses the bounded sprite admission policy from megadev-graphics.
Its single VBlank owner uploads before BIOS INT2/joypad service, then latches
input edges. No AI or sound decisions run in an interrupt. Initial resident art
and palettes load with display off. Clear/game-over panels change only through
immutable VBlank snapshots; movement never blanks/redraws the map.

## Bounds and limits

The search workspace is 500 bytes. Three actors each have a 64-byte route;
the 240-cell queue admits each cell once. Twelve expansions per fixed step,
four catch-up steps and at most four neighbor tests per expansion bound work.
One round-robin sensor runs per fixed step. PAL's occasional second step is
included in these bounds. There is no claim that expansion counts measure CPU
cycles or that three enemies represent the console's maximum.

US/JP/EU release, US debug and target qualification builds use
`ewrogers/megadev:latest`, two container CPUs, base 68000 code and the toolchain's
optimization safeguards. Native ARM64/AMD64 CI compares complete payloads.
Local AMD64 testing uses Docker emulation. CI/distribution installs disable
telemetry. Ares PAL/NTSC checks and final CI results are reported in the PR;
physical console timing, controller and sound validation remain outstanding.

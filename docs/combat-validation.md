# Combat milestone validation

`megadev-combat` adds attack resolution without duplicating the animation,
collision, AI or gameplay owners. The original Copperleaf Sparring Court is an
illustrative two-team example, not a prescribed art style or combat system.

## Reproduce

```sh
python -m unittest discover -s tests -p test_combat.py
python tests/integration.py --skill megadev-combat --platform linux/arm64 --output build/combat-arm64
python tests/integration.py --skill megadev-combat --platform linux/amd64 --output build/combat-amd64
python tests/compare_reports.py build/combat-arm64/report.json build/combat-amd64/report.json
```

Both matrices build US/JP/EU release, US debug and qualification cases through
`ewrogers/megadev:latest`, with `--cpus=2`. Reports record the selected SDK/compiler;
complete target modules and startup results must match. Qualification executes
the same portable contract on original M68000 before returning to normal play.

Host tests use independent overlap and guard-cone oracles, O0/O3 executable rules
and UB checks where supported. Target tests execute those rules and publication:
immutable snapshots, late-frame deferral, invalid counts/indices, no normal CRAM
writes, opaque panels below sprite priority, restart cleanup and finite fanfare.

RAM: five actors and eight bolts, two 1962-byte publication frames, plus the
contract's static fixtures. The CPU harness checks total Main BSS against the
reserved stack region. Art is resident: 359 sprite patterns, 24 ground patterns,
96 font patterns and four palettes. No runtime allocation, disc load or Word RAM
transfer occurs during combat. These are Lab budgets, not engine limits.

Ares acceptance covers controls, visible reach, guards, damage/knockback, opponent
behavior and completion in NTSC and PAL. The PR records which manual checks were
actually confirmed. Two-pad join logic is automated; physical second-pad bindings
require a configured device. CPU execution does not measure cycles or drawn
pixels; emulator tests do not qualify physical-console timing/audio/input.

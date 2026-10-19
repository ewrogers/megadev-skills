# Validation and evidence

Run the standalone `tools/check_combat.py` with a host C17 compiler. It executes
O0/O3 contracts with undefined-behavior checks where supported. Cases include
edge-touch geometry, active-window boundaries, once-per-swing contact, front/rear
blocks, simultaneous trades, hurt interruption, invulnerable contact retirement,
stale/invalid projectile owners, serial wrap, wall-safe knockback, second-pad
joining, bounded long runs, catch-up caps, partial-step input and PAL/NTSC equality.

Repository tests additionally compare overlap and all four guard directions
against independent integer oracles, reproduce generated asset bytes and validate
native dimensions/palette words. Execute the actual compiled contract and the
immutable publisher on original M68000; check stack/RAM separation, deferred late
VBlank, invalid publication/count rejection, SAT/HUD bytes and no CRAM writes.

Build fresh US/JP/EU release, US debug and qualification cases using
`ewrogers/megadev:latest`, `--cpus=2`. Native ARM64/AMD64 CI compares complete target
payloads. Installation tests run the generated Lab's portable checks with telemetry
disabled. A compiler passing a host test does not prove the freestanding target link.

In ares, check readable windup/swing/recovery, shield direction and front/rear
hits, enemy guard gaps, bolts, hurt flashing/knockback, all opponents, clear banner,
fanfare, restart and pause. Repeat PAL and NTSC. A second configured pad can join
with Start; host/CPU tests of join logic do not qualify physical pad bindings.

The CPU harness models VDP registers and does not measure cycles, raster timing,
scanline pixels or sound. Emulator playtests do not qualify physical-console
performance, controller variants, analog audio or real drive latency. Retest the
combined renderer/audio workload on hardware before claiming a performance budget.

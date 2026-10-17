# Validation and troubleshooting

## Build and inspect

Record `doctor`, build with the intended region/video mode and inspect outputs:

```sh
python3 scripts/inspect_disc.py /path/to/my_game.iso --region US --expect IPX.MMD --expect SPX.SMD --expect ROOM.MMD
```

The inspector checks a data-only ISO's filesystem bounds, expected files and selected region's boot signature. Its hashes help compare builds while ignoring ISO metadata timestamps. It is not a mixed-mode/CD-audio validator, burn certification or substitute for booting the disc. The regional signature baseline follows [ares v148's Mega CD medium loader](https://github.com/ares-emulator/ares/blob/v148/mia/medium/mega-cd.cpp).

## Emulator checklist

1. Launch the CUE explicitly as Mega CD, with a matching, user-supplied BIOS.
2. Confirm Main initialized data, Main zeroed RAM and Sub startup show PASS.
3. Confirm both tick counters advance; their values need not match.
4. Move the marker and toggle its hold state with Start.
5. Press B for a ping. Its reply should change without stopping the counters.
6. Press A to load the room. Its own data/BSS checks should pass and resident ticks should continue.
7. Return with A/Start and repeat the load. The load count rises, while the room's private state starts fresh.

## If something fails

| Symptom | Check first |
| --- | --- |
| System selection prompt or capture timeout | Pass `--system "Mega CD"`; do not guess that the game crashed. |
| BIOS screen stays up | Matching region and actual Start mapping. |
| Start works, A/B do not | Mega CD's controller mappings, separately from Mega Drive. |
| Dark/invisible text | Font pixel indices, palette cache and its VBlank upload; nonzero palette entries are white in the Lab. |
| Sub counter stays at zero | Interrupt setup and work after a tail-jumping callback macro. |
| Initializers fail on reload | Module entry, BASIC_INIT, data source/destination and BSS size units. |
| Module hangs after rebuilding | Reload the entire immutable disc; resident exports may differ. |
| Runtime timeout/read failure | Stop using the uncertain buffer and investigate command status/ownership; reset this Lab to retry. |

## Evidence boundaries

Repository tests run portable control/handshake code with failure cases, inspect real disc payloads and execute actual M68000 C startup and the Sub callback in Unicorn. Startup memory is deliberately dirty and has guards. Native ARM64/AMD64 CI compares all regional release and US debug payloads. CPU-only execution mocks memory and a continuation; it does not prove BIOS behavior, both CPUs together, disc timing, interrupts or real peripherals.

Ares checks cover interactive boot and module loading. Real hardware is still needed for drive latency/retries, physical-controller behavior, BIOS/model variation and timing. No BIOS is shipped or fetched by these helpers.

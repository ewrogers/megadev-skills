# Audio validation

Run the installed generator in a fresh directory, then build with
`ewrogers/megadev:latest` on ARM64 and AMD64. Both build/test containers use a
two-CPU quota. Record the actual SDK/compiler revision; compare compiled modules,
boot payloads, disc files **and external CD-audio WAVs**, not timestamped ISOs.

The repository matrix covers US/JP/EU release, US debug and US automatic tour.
Portable C tests run voice/drive policy at O0/O3 with undefined-behavior checking
where supported. Authoring tests check sign/magnitude extrema, no reserved loop
markers in payload, sample bank bounds, release tails, pitch rounding, PCM/WAV
output, real pregap bytes and CUE preservation/rejection.

Executed original-68000 tests run the actual Main/Sub adapters. The PCM model
checks byte lanes, wave-bank addressing, preserved channel masks, pitch/pan
registers and byte-for-byte sample upload. A BIOS completion stub checks the
C ABI track pointer, command ordering, pause/resume, data-lock exclusion,
latest-track restart, protected effects, panic and stuck-command quarantine.
Separate paired reader tests cover ownership, delayed/failing loads, retirement,
corrupt data and copied VBlank payloads. These models do not synthesize sound or
simulate BIOS seek timing. A command log is not proof of audible correctness.

## Listen in ares

Open the mixed-mode CUE as **Mega CD**, using a locally configured BIOS.
Opening only the ISO cannot test external CD-audio tracks. Check:

- CD MUSIC: both tracks, repeat, pause/resume and stop. Each track's start should
  survive its pregap; stopping must not leave a stuck sound.
- PCM DESK: bell/pluck/drum, center/left/right pan, finite release and independent
  looping bed. Holding the button must not retrigger. Listen for seams/clipping.
- DRIVE TEST: keep the PCM bed on, play CD music, then load another area. CD music
  intentionally stops and restarts from the beginning; bed and effect requests
  should remain usable. Start silence during/after a read must not be undone by
  stale music intent.
- PAL and NTSC: CD tracks retain pitch/tempo; PCM pitch stays the same and timed
  effect release stays in seconds. Displayed VBlank count itself differs.

The automatic tour submits music, effects, panning, pause/resume and a read,
then checks data-load ownership with the PCM bed active. It is not a substitute
for listening or for physically configured input checks.

## Remaining physical evidence

Test real console/BIOS/drive models, cold/warm seeks, repeated music/data handoffs,
CD-DA track boundaries/repeat/pregap behavior, actual PCM pitch/mix headroom,
voice stealing and electrical/bus timing. Qualify any future FM/Z80 driver,
PCM streaming or ASIC/DMA coexistence separately. Ares behavior does not prove
all Sega CD hardware behaves identically.

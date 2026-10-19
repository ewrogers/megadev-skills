# Streamed PCM validation

This expands `megadev-audio`, not the skill inventory. Its existing Systems
entry in `skills.sh.json` remains correct. All commands use
`ewrogers/megadev:latest`, two Docker CPUs and fresh generated projects.

## Scope and audited support

The streamer uses the selected SDK's asynchronous `CDROM_LOAD_CDC` operation,
real `find_file` directory lookup and RF5C164 registers. It does not claim a
Megadev streaming API or concurrent CD-DA/data seeks. Details and primary links
are in [the skill reference](../skills/megadev-audio/references/streamed-pcm.md).

The pulled image reports SDK `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`,
GCC 16.2.0 and original-68000 defaults including `-fno-auto-inc-dec` and
`-fno-strict-aliasing`. Source/revision checks precede API guidance.

| Clip | Encoded sample bytes | Duration at 8 kHz | 16 KiB disc chunks |
| --- | ---: | ---: | ---: |
| Original synthesized speech | 165611 | 20.70 s | 11 |
| Procedural chimes | 112000 | 14.00 s | 7 |

The final chunk is zero-padded. Runtime memory stays at two 16 KiB PCM halves
and one 16 KiB Sub staging buffer for either length. Resident PCM occupies
another 16 KiB and remains unchanged. The US release links 16524 bytes of Sub
BSS and 20 initialized bytes in its explicit 32 KiB data reservation; the
sample constants remain in the Sub module. The Sub stack is separate.

The checked-in voice WAV is band-limited/resampled offline. A fresh build uses
Python's standard library, not a speech service or platform speech executable.
Asset provenance and the utterance are committed alongside the source WAV.

## Automated evidence

- Portable streaming policy at O0/O3, with UBSan where supported: half ownership,
  order, first prefill, completion, silence/rebuffer, final single chunk and
  impossible playhead/sequence rejection.
- Authoring: exact sector size, FF only at the deliberate end marker, clips
  larger than all PCM RAM, deterministic chunk regeneration, damaged-output
  repair and rejected WAV format/rate. No full-clip runtime allocation.
- Actual compiled original-68000 Main/Sub adapters with modeled BIOS/CDC and
  fractional PCM address progression: whole-clip order/content, no upload into
  a playing half, the 512-byte cap, preserved resident samples/effects, latest
  music return and reader/voice mutual exclusion.
- Failure paths: intentional slow upload followed by silence and resumption,
  cancellation before CD-DA stop completion and during a live read, replacement
  refusal while draining, missing/oversized files, I/O/short reads, bad markers,
  checksum damage, torn address reads, excessive worker gaps and a read that
  times out but retains its buffer until actual retirement.
- Existing mixed-mode validation/package round trip includes every PCM chunk,
  alongside both CD tracks and all compiled modules. Missing executed voice
  evidence or chunk files fails report comparison.
- Telemetry-disabled installed-skill validation includes the voice source,
  chunk generator and both kinds of chunk. The full host suite has 175 tests.

The target matrix remains US/JP/EU release, US debug and US tour for both
ARM64/AMD64. Local AMD64 uses Docker emulation on the ARM Mac; GitHub Actions
uses native runners for both architectures. Compare target modules, all disc
payloads, boot binaries and external audio, rather than timestamped ISO files:

```sh
python tests/integration.py --skill megadev-audio --platform linux/arm64 --output build/audio-arm64
python tests/integration.py --skill megadev-audio --platform linux/amd64 --output build/audio-amd64
python tests/compare_reports.py build/audio-arm64/report.json build/audio-amd64/report.json
```

The existing tour exercises CD-DA/effects/data handoffs. Streamed playback is
covered by the additional CPU adapter checks and manual audition; the old tour
PASS message alone is not evidence that the long voice was heard.

## Emulator and hardware qualification

NTSC ares audition confirmed the full speech, resident effect, moving display
and return to CD music. Repeat with the EU/PAL build: Left opens
VOICE STREAM, A plays, Up selects the other clip, B plays an effect and C stops.
Down applies the intentionally insufficient upload cap to the next start;
buffering is slower and silence/rebuffering is expected.

The subsequent EU/PAL ares audition also passed: speech, long chimes, resident
bell, music return and display were confirmed. This is listening/display
evidence, separate from the target model/build checks. PCM pitch is
driven by the chip clock; upload opportunities differ between 50/60 Hz. A
recorded emulator success is not a physical-drive timing guarantee. Cold seeks,
worn media, mix headroom, real PCM register timing and concurrent ASIC/VRAM
workloads still need console checks. This two-half streamer cannot tolerate an
arbitrary unserviced CPU stall; it stops on an observed excessive worker gap.

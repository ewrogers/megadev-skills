# Evidence and limits

## Executable checks

- Host C O0/O3 contract examples: subtraction-based bounds, odd-offset byte
  decoding, invalid inputs, 16-bit clock wrap, capped gaps and equal nominal
  four-second PAL/NTSC elapsed time. Undefined-behavior sanitizer is used by
  repository tests where the host supports it.
- Standalone installation: create Harbor Relay without other skills installed;
  run `tools/check_contracts.py` with a host C17 compiler and retain the bundled
  release helper. Installation checks disable telemetry.
- Ownership regression: shared scene/stream/audio implementations, save codec
  and release helper remain byte-identical to their owner examples. The BRAM
  adapter differs only in its private save filenames.
- Original-68000 execution: actual C/ABI adapters with shared register state and
  a named-file BIOS model. Checks include copied save inputs, busy rejection,
  reply retirement, drive-operation exclusion, failed/unavailable media, older
  record recovery, scene lease barriers, stale Continue, post-map restoration
  and timeout quarantine. No format/delete operation is accepted.
- Existing compiled scene, CD/audio, map-upload and callback regressions remain
  active. Release checks package/verify the mixed-mode disc and reject broken
  tracks, changed indices/paths, damaged boot bytes and damaged archives.
- Native ARM64/AMD64 CI builds US/JP NTSC, EU PAL, debug and non-writing automatic
  tours. Complete payloads, track semantics and execution/release evidence must
  match. Whole-ISO timestamp reproducibility is not claimed.

The BIOS/drive/VDP models test decisions and ABI behavior, not hardware latency,
optical media, interrupt timing or analog audio. Host arithmetic checks alone do
not qualify a target build, and emulator success does not certify every BIOS/model.

## Interactive acceptance

Start Harbor Relay, fly to the white-and-gold beacon and press A to travel. B
preloads; returning to title while it is active should drain safely. C on title
intentionally tests missing-area recovery. Music should stop for data loading
and return; effects and fades should stay correct.

In a world scene, Start pauses, B explicitly saves, and C returns to title.
Choose Continue to restore the saved area, horizontal position, flight option and
beacon progress. Change areas after saving before trying Continue, so a stale
in-memory state cannot masquerade as a successful load. Never format existing
media just to pass this check. Missing/unformatted media is a valid reported result.

Repeat relevant checks on NTSC and PAL, and test the actual freshly unpacked
release. Automatic tours do not write backup RAM and cannot replace the manual
checkpoint check. Save BIOS duration, music/PCM continuity during a real save,
physical seek latency, late VBlank behavior, stack high-water and console/BIOS
compatibility require explicit hardware evidence. Reuse the hardware, performance
and release owners' qualification plans rather than claiming coverage here.

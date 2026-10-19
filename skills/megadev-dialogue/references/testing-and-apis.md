# Verified APIs and validation

Developed against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0.
Pull `ewrogers/megadev:latest` and inspect its selected source after image updates.
Public examples retain base-68000, aliasing and compiler safeguards.

Primary Megadev sources:

- [main/bios.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h) and [main/bios.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h): joypad state, register cache, display/DMA and Sub INT2 services.
- [main/vdp.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.h): port/command encoding used by the existing UI publisher.
- [sub/pcm.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.h) and [sub/pcm.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.def.h): PCM banks, channels, start/loop/pitch and odd-byte sample RAM access.
- [sub/bios.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bios.def.h): CD BIOS entry points retained by the audio adapter.

`d_*`, `DSpan`, `Speech`, `speech_*`, `audio_request` and `AU_SPEAK` are application
example APIs, not Megadev dialogue/mixer facilities. The adapter preserves C ABI
registers and the existing Sub INT2 path. Pass computed bank indices to the SDK
macros carefully; inspect macro expansion rather than assuming argument grouping.

## Automated evidence

- Host O0/O3: bounded word wrapping/paging/styles, rejected output unchanged,
  punctuation delays, ready-generation gate, PAL/NTSC equal time, held actions,
  both branches, paired panels, pause/skip and stale-voice retirement.
- Target: original-MC68000 model/publisher execution, invalid states, both story
  paths and exact bounded row writes. The actual compiled tour runs both stories.
- Paired Main/Sub: sample-byte/bank validation, channel/pitch/loop setup, finite
  stop, speech-only cancellation, independent effects, stale starts, rejection,
  and acknowledgement timeout quarantine. Existing audio policy/BIOS tests remain.
- Native ARM64/AMD64: US/JP NTSC, EU PAL, debug and tour, matching payloads.
- Installed standalone generator, deterministic art/PCM conversion, native PNG
  previews from actual C state, and fail-closed matrix reports. Telemetry stays off.

The preview exchanges raw map words through a binary file, including on Windows.
It needs a host C compiler; the generators/PNG encoder use Python's standard
library. Speech authoring is optional and separate from deterministic conversion.

The user confirmed NTSC and final PAL behavior in ares: dialogue, both choices,
paired panels, voice, pause and returning during speech. For future changes,
repeat those checks and the more-page indicator/C voice toggle. CPU/MMIO models do not emulate video,
PCM acoustics, BIOS/drive timing or interrupt budgets. Physical-console and BIOS
variation checks remain separate; no real-console result is claimed.

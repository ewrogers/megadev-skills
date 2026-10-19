# Megadev timing integration

Audited against the selected image's Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Pull `ewrogers/megadev:latest` before
qualification and retain the actual image/toolchain identity. Recheck source
contracts when the image changes.

| Facility | Verified source and meaning |
| --- | --- |
| Stopwatch | `lib/main/gate_arr.h` exposes `ga_reg_stopwatch`; `gate_arr.def.h` maps Main `0xA1200C`, Sub `0xFF800C`, low twelve data bits. SDK documentation gives a nominal 30.72 microseconds/count. |
| Shared clock | Sub may reset the stopwatch by writing zero. SDK describes CD timing use. Reading the same hardware does not establish an independent profiler-owned clock. The Lab never writes it. |
| Main frame service | The existing bridge preserves registers, triggers Sub INT2, polls BIOS joypads and invokes the Lab VBlank callback. `main_ticks` counts callbacks serviced, not every physical frame under arbitrary interrupt masking. |
| ASIC/ownership | Existing `scale_*` adapters poll actual operation completion, return Word RAM and copy to a resident Main snapshot before publication. These are Lab adapters, not SDK profiling APIs. |
| Transfer path | The existing `lab_upload_work` C/BIOS bridge uploads at most 2048 words per early VBlank, deferring outside the supported blank window. |
| Regions | `VIDEO=NTSC/PAL` selects the Lab's nominal 60/50 Hz. This label is a build choice, not live hardware-rate detection. |

Primary sources: [Main register definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/gate_arr.def.h),
[Sub register definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.def.h),
and [Megadev](https://github.com/drojaazu/megadev).
The installed source in the selected image is the integration authority.

Do not substitute SGDK CPU-load helpers or frame callbacks. Existing timers and
BIOS/CD/audio owners must agree on clock use. In a game that resets the stopwatch,
use a qualified alternative, stop the conflicting measurement or report it as
unavailable. Two reads cannot detect every external reset.

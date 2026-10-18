# Actual Megadev integration

Audited from the selected container's Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Pull `ewrogers/megadev:latest` before
qualification and retain `doctor` output; recheck symbols and assembly contracts
when the image changes.

| Facility | Verified source / boundary |
| --- | --- |
| Input | `lib/main/io.def.h` defines PAD_UP/DOWN/LEFT/RIGHT as low four bits. `lib/main/bios.h` exposes `bios_joy1_hold` / `bios_joy1_hit`. The existing Main bridge calls BIOS_READ_JOYPAD. |
| Main/Sub service | Main bridge calls BIOS_TRIGGER_IFL2; the Sub bootstrap keeps its INT2 access-loop callback. Test code must not mask or replace this service while measuring loading. |
| CD I/O | Existing adapters use `sub/cdrom.h`, the directory cache and access operation/result fields. CDC completion and ownership return are distinct from upload readiness. |
| Audio | Existing Sub wrappers issue BIOS CD music operations and manage PCM registers; Main submits application commands. Those wrappers and `audio_*` are Lab code, not SDK API names. |
| Region/build | `REGION` and `VIDEO` are build selections in `megadev.make`. Test Lab's Hz label reflects its compiled VIDEO target, not a hardware measurement. |
| Startup/modules | Megadev links separate Main MMD/Sub SMD modules and the standard boot programs. Each CPU has its own addresses, data initialization and BSS. |

Primary sources: [Megadev build rules](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/megadev.make),
[input definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.def.h),
and [Megadev source](https://github.com/drojaazu/megadev).
The image's installed source is the integration authority. No SGDK APIs or resource
formats are dependencies of this skill.

The fixed-tick clock, replay structures, suite state machine and MMIO/BIOS test
models are application/test policy. The examples do not claim a built-in Megadev
replay system, remote ares QA API or hardware-accurate Gate Array simulation.

# Verified Megadev boundaries

Audited against Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` through
`ewrogers/megadev:latest`; recheck the selected image before adapting APIs.

| Facility | Actual source and meaning |
| --- | --- |
| Linked symbols | `megadev.make` emits `build/<module>.elf` and `<module>.elf.sym` with `nm -n`. `DEBUG` contributes `-DDEBUG`; debug information requires an explicit `-g` build flag. |
| Main exception helper | `lib/main/cd_exception.s`, `lib/main/str_util.s` and Main `EXVEC_*` definitions. Optional assembly, not automatically installed by a build macro. |
| Interrupt service | Main custom bridge preserves registers, calls `BIOS_TRIGGER_IFL2` and `BIOS_READ_JOYPAD`, then the Lab callback. Sub's existing access-loop callback continues CD service. |
| Mailboxes/ownership | Main/Sub `gate_arr.h` views, `ga_reg_comcmd*`, `ga_reg_comstat*`, `ga_reg_memmode` and distinct Word RAM views. The Lab reuses `megadev-system`'s portable `coord.c` unchanged. |
| Module relocation | `cfg/module_mmd.ld`, MMD header, `init_mmd` and module startup. Runtime code addresses differ from file/ISO offsets. |

Primary sources: [build rules](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/megadev.make),
[Main exception helper](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/cd_exception.s),
[module linker](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/cfg/module_mmd.ld),
and the [Megadev repository](https://github.com/drojaazu/megadev).
The source in the pulled image is authoritative for that build; public commands
use `latest`, while reports retain the actual compiler/SDK identity.

The container provides GDB and binutils. Their presence does not establish a
remote connection to ares, a dual-CPU stop model, or hardware debugger support.
Offline `nm`, `addr2line`, disassembly and ELF inspection work without a remote
stub. Verify a particular emulator's debugger facilities before suggesting commands.

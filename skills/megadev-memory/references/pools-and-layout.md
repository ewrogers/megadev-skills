# Pools and linked placement

Audited with `ewrogers/megadev:latest`, Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0. Pull latest and inspect its actual source/doctor output again when adapting.

| Resource | Physical capacity | Placement concern |
| --- | --- | --- |
| Main Work RAM | 64 KiB | Resident Main code/constants/state, stack, vectors and any Main BIOS workspace |
| Sub Program RAM | 512 KiB | Sub BIOS/system program, access service, modules, state and stack |
| Word RAM | 256 KiB | One owned 2M region or two separately assigned 128 KiB banks |
| Sega CD PCM wave RAM | 64 KiB | Sample/channel banking and audio ownership; not a C heap |
| Z80 RAM | 8 KiB | Sound code/state; bus ownership still matters |
| VDP VRAM | 64 KiB | Patterns, maps, SAT and scroll tables; not CPU-local RAM |

CRAM/VSRAM and backup RAM are also separate facilities, not extra general allocation space. Save-media capacity/format policy belongs to save guidance.

See Megadev's [hardware overview](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/megacd_dev.md), [Main map](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/memmap.def.h), [Sub map](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/memmap.def.h) and [modules](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/modules.md). Main's banked Program RAM window is not unrestricted concurrent access to Sub memory. This Lab leaves Sub Program RAM under Sub control.

## What the report measures

From a generated Memory Lab, after building:

```sh
python3 tools/report_layout.py --root .
```

The script reads `layout.json` and `build/*.elf.sym`. It checks physical CPU capacities, reservation overlaps, module origins/capacities, actual linked extents and embedded pool bounds. It rejects missing symbols and one-byte overruns. It never counts a source-declared array that the compiler optimized away as real RAM usage.

`MODULE_ROM_*` names describe a module's load/code region; on this disc it resides in RAM. The report conservatively includes the initialized-data source image there, and the copied initialized data in the writable state region. BSS consumes RAM even though it adds no equivalent zero-filled bytes to the disc payload.

Reserved bytes and linked bytes answer different questions. A 40 KiB code window containing 13 KiB of code is not a 27 KiB heap until the linker/resource policy explicitly makes it one. Reclaim margins by changing the layout and validating all affected regions, not by pointing an allocator into a gap discovered at runtime.

## This Lab's policy

Main reserves code at FF0000–FF9FFF, state at FFA000–FFCFFF, stack at FFD000–FFDFFF, a room-fixture state region at FFE400–FFEBFF and Main BIOS workspace at FFF700–FFFFFF. That is 61,696 reserved bytes and 3,840 unassigned bytes. The scene arena's 4,096 bytes are embedded in linked state, not added again.

Sub reserves the lower 128 KiB for BIOS/bootstrap/access service, module code/state, growth margin and stack. The upper 384 KiB is deliberately unassigned. These broad starter windows demonstrate accounting; they are not an engine tax or a statement that a game needs all those bytes.

ROOM.MMD remains a startup-test fixture and is not loaded by the runtime memory demo. Its Main state reservation is counted conservatively; its possible Word RAM execution image is not a second simultaneously resident payload.

Stacks here are bounded reservations, not measured maxima. Production budgets need stack high-water/guard checks under realistic interrupts and BIOS/audio work. Neither this report nor a successful link proves those timing/lifetime properties.

---
name: megadev-hardware
description: Investigate and validate Sega CD/Mega CD hardware behavior in a Megadev project, including CPU-specific memory maps, Word RAM ownership, BIOS services and hardware test evidence. Use for hardware assumptions and diagnostics rather than game design or SGDK API guidance.
---

# Megadev hardware

Identify the specific hardware contract the project relies on, then build the smallest safe check for it. Preserve the user's game, renderer, audio policy and controls. Hardware Lab is a diagnostic example, not a game template to impose on their project.

## Establish what is running

Record the build region/video mode, Megadev revision, toolchain and emulator/console setup. Inspect the selected `ewrogers/megadev:latest` source before teaching an unfamiliar register or API. Source citations in these references identify the tested baseline, not every future image.

Read [machine contracts](references/machine-contracts.md) for CPU-specific maps, ownership and register access. Read [verification](references/verification.md) when selecting tests or reporting results.

## Check the contract, not just the screen

- Distinguish Main Work RAM, Sub Program RAM, shared Word RAM, PCM wave RAM and VDP memory. A pointer or free-byte count is not proof of access.
- In 2M mode, establish ownership before touching Word RAM. In 1M mode, account for each bank and its view separately. Stop executing replaceable code and quiesce callbacks before changing ownership or contents.
- Preserve BIOS workspace, code, stacks and active audio/graphics buffers. Test only explicitly reserved scratch space. A whole-RAM sweep in a running program is destructive to the program itself.
- Respect original-68000 alignment, access widths, ABI and interrupt contracts. Register reads can have side effects; do not use arbitrary memory scans on I/O.
- Keep command arguments stable until the acknowledgment protocol allows reuse. Ownership, acknowledgment and completion are separate facts.
- Keep base-console VDP, Z80 and bus constraints in view; the add-on does not remove them. Audio/ASIC/streaming implementations need their own measured system tests.

## Use the Lab when useful

[Hardware Lab](assets/hardware-lab/README.md) is a self-contained disc with bounded scratch-pattern primitives, a 2M round trip, ping and module reloads. `scripts/create_project.py` creates a new copy without replacing existing files. Its optional automatic tour exercises runtime operations when emulator input automation is unavailable; it does not test physical controller mappings.

Use `scripts/inspect_disc.py` for this data-only image's region/filesystem checks. The helper is not a full mixed-mode release validator. Building/bootstrapping a new game belongs to `megadev-project`; feasibility and lifetime budgets belong to `megadev-planning`. This skill owns the hardware assumptions and evidence, not a rival project architecture.

## Report evidence precisely

Separate source-backed facts, host-model checks, actual CPU execution, emulator observations and physical-console results. A passed scratch buffer is not a complete RAM test. A nominal PAL flag is not measured frame timing. A data-only disc says nothing about CD-DA coexistence. State untested modes and peripherals explicitly, and choose a focused next proof rather than claiming the whole machine is qualified.

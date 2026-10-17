---
name: megadev-planning
description: Plan a Sega CD/Mega CD game using Megadev, with CPU responsibilities, memory lifetimes, Word RAM ownership, disc loading and hardware budgets. Use for feasibility, architecture and milestone planning before or during implementation.
---

# Megadev planning

Turn the user's game idea into a small, testable plan. Keep their setting, art, audio, mechanics and controls. Teaching examples illustrate hardware decisions; they are not game designs to copy.

## Start with what must work

Inspect the project before proposing a new architecture. Identify its target regions, current renderer/audio services, resident code, module loader and evidence from previous tests. Ask about unknown player-facing requirements only when they change feasibility. Use provisional estimates for the rest and label them.

Choose the smallest milestone that proves the main risk: for example, loading the next room without interrupting input or audio. A planning task does not require a new ROM, Docker installation or emulator configuration unless that proof is part of the request.

## Make the constraints visible

Read [hardware budgets](references/hardware-budgets.md) for sourced capacities and the distinction between shared base-console limits and Mega CD additions. Treat physical ceilings, framework reservations, project allowances and measured usage as different things.

Plan these together:

- Main/Sub responsibilities and communication. A second CPU is not a transparent second thread for the Main frame loop.
- Resident code/data, replaceable modules and temporary buffers. Include C startup, stacks, BIOS space, packed input and decoded output at their peak overlap.
- Word RAM mode and owner during each phase. Keep code and callbacks reachable while memory changes hands.
- VDP tables, patterns, palettes, crowded scanlines and upload time. Extra RAM does not increase VRAM or sprite limits.
- Audio residency and disc access. Decide what happens to CD-DA while reading data, and protect active PCM sample data from refill/replace operations.
- Late/failed loads and save-media failures, when relevant. A plan needs a fallback, not an assumed perfect drive.

Use [the workflow](references/planning-workflow.md) to choose trade-offs and define proof tasks. Build/API implementation belongs with `megadev-project` or the relevant system work; do not import SGDK APIs, mapper assumptions or XGM drivers.

## Produce an editable plan

Copy the useful parts of [game-plan.md](assets/game-plan.md) into the project. Explain the decision and its uncertainty in plain language. Avoid making the user tune every theoretical limit before there is a measured reason.

For arithmetic, adapt [budget-example.json](assets/budget-example.json) and run:

```sh
python3 scripts/budget_check.py /path/to/budget.json
```

Read [the budget format](references/budget-format.md) before editing its phase/ownership model. The helper checks only declared allocations and demands; PASS does not certify a layout, performance, a handshake or physical-console behavior. Its optional load calculation uses explicit assumptions and always says so.

Close with the next executable proof, a pass criterion, and what still needs an emulator or real hardware. Inspect the selected Megadev source before promising an unfamiliar API; the reference baseline is evidence, not a substitute for the installed version.

# Working on Megadev Skills

- These skills target Megadev and Sega CD/Mega CD development. Do not copy SGDK
  APIs, resource formats, drivers or cartridge assumptions into Megadev guidance.
- Use `ewrogers/megadev:latest` in public commands and pull it before validation.
  Record actual compiler/SDK revisions in test reports; inspect the selected
  image's source before teaching an API or changing an assembly contract.
- Keep hardware facts, BIOS services, Megadev helpers and optional game design
  conventions distinct. Account for both CPUs, RAM ownership, interrupts and
  VDP budgets. The sub-CPU and ASIC are not unlimited background resources.
- Keep each installed skill self-contained. Detailed guidance belongs in linked
  references, and executable examples must work without repository-level helpers.
- Compare related skills and record ownership seams. Reuse the game's clocks,
  renderer, resource policy and input decisions rather than introducing rivals.
- Labs demonstrate techniques, not prescribed art, sound, controls, progression
  or game architecture. Adapt the smallest useful part to the user's brief.
- Test useful behavior/failure cases, compile for the original 68000, and compare
  target payloads on native ARM64/AMD64 CI. Distinguish host models, CPU execution,
  emulator checks and physical-console evidence. Never overstate timing coverage.
- Keep BIOS files, commercial images, generated discs, credentials and tool
  downloads out of Git. Preserve upstream notices. Do not auto-format save media.
- Keep README, skills.sh groups, references, examples and test coverage in sync.
  CI installation checks disable telemetry; refresh telemetry only when requested.
- Use PRs for review. Do not merge on the user's behalf or overwrite projects,
  source art, emulator settings or existing save data.

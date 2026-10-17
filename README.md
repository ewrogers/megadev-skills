# Megadev Skills

Agent skills for building Sega CD / Mega CD games with [Megadev](https://github.com/drojaazu/megadev).
They teach practical hardware and SDK techniques, backed by executable examples and tests.

Use our [Megadev Docker toolchain](https://github.com/ewrogers/megadev-docker) on macOS, Linux or Windows with Linux containers. Images support ARM64 and AMD64.

## Install

```sh
npx skills add ewrogers/megadev-skills --skill megadev-project
```

Ask your agent to use `megadev-project` to create or troubleshoot a project.
The skill includes a standalone starter and instructions for building a disc with `ewrogers/megadev:latest`.

## Skills

| Group | Skill | What it covers |
| --- | --- | --- |
| Foundations | [megadev-project](skills/megadev-project/SKILL.md) | Toolchain setup, Main/Sub startup, memory layouts, module loading and a bootable disc. |
| Foundations | [megadev-planning](skills/megadev-planning/SKILL.md) | CPU responsibilities, memory lifetimes, Word RAM ownership and loading plans, with an editable budget checker. |
| Foundations | [megadev-hardware](skills/megadev-hardware/SKILL.md) | CPU memory views, ownership and safe diagnostic checks, with a Hardware Lab disc. |

| Foundations | [megadev-assets](skills/megadev-assets/SKILL.md) | Native PNG preparation, palettes, patterns and sprite sheets, with validated disc assets and Word RAM uploads. |

More skills will follow in foundations, systems, advanced techniques, debugging/testing and release. See the [adaptation roadmap](docs/skill-adaptation-roadmap.md).
For cartridge games using SGDK, see [SGDK Skills](https://github.com/ewrogers/sgdk-skills).
Megadev has its own APIs and build conventions; these skills do not require SGDK.

## Examples and validation

[Project Lab](skills/megadev-project/assets/project-lab/README.md) is a small interactive disc that checks startup, communication and module loading.
[Hardware Lab](skills/megadev-hardware/assets/hardware-lab/README.md) adds safe scratch-buffer checks and a Word RAM handoff test, with an optional automatic runtime tour.
[Asset Lab](skills/megadev-assets/assets/asset-lab/README.md) reloads two original PixelLab backgrounds from disc while keeping a shared animated sprite and UI resident.
Labs demonstrate implementation techniques. Their appearance, controls and game design are examples, not requirements for your game.

[Validation notes](docs/validation.md) explain the host tests, executed 68000 startup checks, architecture comparison and emulator checks. Emulator success does not replace real-console testing.

## License and disclaimer

Our code and documentation are [MIT licensed](LICENSE). Upstream material retains its own notices, including the starter's `LICENSE.megadev`.

This is an independent community project, not affiliated with, endorsed by or sponsored by Sega. Sega, Mega Drive, Genesis, Sega CD and Mega CD are trademarks of their respective owners. No Sega BIOS files or commercial games are included.

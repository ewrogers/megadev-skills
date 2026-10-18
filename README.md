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
| Foundations | [megadev-input](skills/megadev-input/SKILL.md) | Pad, mouse and multitap protocols, stable input snapshots and joining, with explicit device-validation limits. |
| Foundations | [megadev-graphics](skills/megadev-graphics/SKILL.md) | Scrolling, parallax, sprites, animation frames and palette effects, with bounded frame submission. |
| Systems | [megadev-audio](skills/megadev-audio/SKILL.md) | CD music, PCM samples, voice priorities and safe music/data loading handoffs. |
| Systems | [megadev-memory](skills/megadev-memory/SKILL.md) | Linked RAM budgets, allocation lifetimes, buffer leases and safe 1M/2M Word RAM handling. |
| Systems | [megadev-save](skills/megadev-save/SKILL.md) | Internal backup RAM, versioned records, verified writes and corruption recovery, with cartridge boundaries. |
| Systems | [megadev-scenes](skills/megadev-scenes/SKILL.md) | Game flow, fades, safe scene cleanup, preloads and failed-transition recovery. |
| Systems | [megadev-streaming](skills/megadev-streaming/SKILL.md) | Safe CD loads, preloading, resident art and bounded uploads with Word RAM handoffs. |
| Systems | [megadev-system](skills/megadev-system/SKILL.md) | Main/Sub command lifetimes, bounded work, safe Word RAM handoffs, cancellation and late-reply recovery. |

More skills will follow in foundations, systems, advanced techniques, debugging/testing and release. See the [adaptation roadmap](docs/skill-adaptation-roadmap.md).
For cartridge games using SGDK, see [SGDK Skills](https://github.com/ewrogers/sgdk-skills).
Megadev has its own APIs and build conventions; these skills do not require SGDK.

## Examples and validation

[Project Lab](skills/megadev-project/assets/project-lab/README.md) is a small interactive disc that checks startup, communication and module loading.
[Hardware Lab](skills/megadev-hardware/assets/hardware-lab/README.md) adds safe scratch-buffer checks and a Word RAM handoff test, with an optional automatic runtime tour.
[Asset Lab](skills/megadev-assets/assets/asset-lab/README.md) reloads two original PixelLab backgrounds from disc while keeping a shared animated sprite and UI resident.
[Graphics Lab](skills/megadev-graphics/assets/graphics-lab/README.md) adds a scrolling harbor, animated airships, foreground occlusion and palette effects.
[Input Lab](skills/megadev-input/assets/input-lab/README.md) shows live device snapshots and a separately labeled scripted peripheral tour.
[System Lab](skills/megadev-system/assets/system-lab/README.md) demonstrates bounded Sub work, rejected requests and safe cancellation/timeout recovery while Main keeps running.
[Memory Lab](skills/megadev-memory/assets/memory-lab/README.md) visualizes RAM reservations, leased allocation lifetimes and verified 1M bank exchanges.
[Streaming Lab](skills/megadev-streaming/assets/streaming-lab/README.md) preloads a second area while the current view stays resident, then travels through a fade with safe cancellation and missing-file recovery.
[Audio Lab](skills/megadev-audio/assets/audio-lab/README.md) auditions original CD tracks and PCM effects, with stereo panning and explicit music/data drive handoffs.
[Transition Lab](skills/megadev-scenes/assets/transition-lab/README.md) brings title, options, two areas, pause, results and credits together with safe preload cancellation and recovery.
[Save Lab](skills/megadev-save/assets/save-lab/README.md) saves versioned BIOS files, verifies readback and recovers an older record after an explicit corruption test.
Labs demonstrate implementation techniques. Their appearance, controls and game design are examples, not requirements for your game.

[Validation notes](docs/validation.md) explain the host tests, executed 68000 startup checks, architecture comparison and emulator checks. Emulator success does not replace real-console testing.

## License and disclaimer

Our code and documentation are [MIT licensed](LICENSE). Upstream material retains its own notices, including the starter's `LICENSE.megadev`.

This is an independent community project, not affiliated with, endorsed by or sponsored by Sega. Sega, Mega Drive, Genesis, Sega CD and Mega CD are trademarks of their respective owners. No Sega BIOS files or commercial games are included.

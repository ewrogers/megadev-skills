# Megadev Skills

[![skills.sh](https://skills.sh/b/ewrogers/megadev-skills)](https://skills.sh/ewrogers/megadev-skills)

Practical agent skills for making Sega CD and Mega CD games with
[Megadev](https://github.com/drojaazu/megadev). Build through the
[Megadev Docker toolchain](https://github.com/ewrogers/megadev-docker), keep your
project on your computer, and use an agent to help with setup, assets and debugging.

The playable Labs demonstrate techniques, not designs to copy. Your game's
rules, flow, controls, art and music come from your brief. Use a Lab to examine
a specific solution; building a new game does not mean starting from its code.

For cartridge games using SGDK, see [SGDK Docker](https://github.com/ewrogers/sgdk-docker)
and [SGDK Skills](https://github.com/ewrogers/sgdk-skills). Megadev has its own APIs
and build conventions; these skills do not require SGDK.

## Skills

### Get started

| Skill | What to ask it for |
| --- | --- |
| [megadev-project](skills/megadev-project/SKILL.md) | Start a project, build a disc, check SDK APIs and optionally upload to Mega EverDrive Pro. |
| [megadev-planning](skills/megadev-planning/SKILL.md) | Turn a game idea into a first milestone, CPU responsibilities, memory budgets and loading plans. |
| [megadev-best-practices](skills/megadev-best-practices/SKILL.md) | Write or review C/assembly contracts, module boundaries and safe integration between services. Includes Harbor Relay. |

### Build your game

| Skill | What to ask it for |
| --- | --- |
| [megadev-assets](skills/megadev-assets/SKILL.md) | Prepare native art, palettes, patterns and sprite sheets with validated disc assets. |
| [megadev-graphics](skills/megadev-graphics/SKILL.md) | Scroll backgrounds, add parallax, animate sprites and use palette effects. |
| [megadev-ui](skills/megadev-ui/SKILL.md) | Build fonts, panels, menus and HUDs with bounded redraws. Includes Menu Lab. |
| [megadev-animation](skills/megadev-animation/SKILL.md) | Add named clips, frame holds, timed events and gameplay hitboxes. Includes Animation Lab. |
| [megadev-gameplay](skills/megadev-gameplay/SKILL.md) | Build responsive movement, collision, cameras and bounded object lifetimes across PAL/NTSC. Includes Adventure Lab. |
| [megadev-dialogue](skills/megadev-dialogue/SKILL.md) | Add portrait conversations, timed styled text, choices and safe optional PCM voice. Includes Dialogue Lab. |
| [megadev-input](skills/megadev-input/SKILL.md) | Handle pads, mouse, multitaps and joining with explicit device-validation limits. |
| [megadev-audio](skills/megadev-audio/SKILL.md) | Play CD music and PCM effects, manage voices and coordinate music with data loading. |
| [megadev-scenes](skills/megadev-scenes/SKILL.md) | Connect game flow, transitions, preloads and cleanup, including failed-load recovery. |
| [megadev-save](skills/megadev-save/SKILL.md) | Save to internal or cartridge backup RAM, version records and recover corrupted saves. |

### Work with Mega CD hardware

| Skill | What to ask it for |
| --- | --- |
| [megadev-hardware](skills/megadev-hardware/SKILL.md) | Understand CPU memory views, ownership and safe hardware diagnostics. |
| [megadev-system](skills/megadev-system/SKILL.md) | Coordinate Main/Sub work, cancellation, timeouts and late replies. |
| [megadev-memory](skills/megadev-memory/SKILL.md) | Account for RAM, allocation lifetimes and safe 1M/2M Word RAM handling. |
| [megadev-streaming](skills/megadev-streaming/SKILL.md) | Load and preload CD content with resident art and bounded uploads. |
| [megadev-scaling](skills/megadev-scaling/SKILL.md) | Use the graphics ASIC for rotation, scaling and a perspective floor, with measured limits. |

### Test, debug and optimize

| Skill | What to ask it for |
| --- | --- |
| [megadev-testing](skills/megadev-testing/SKILL.md) | Record replays, compare PAL/NTSC, check integration and capture console video. |
| [megadev-debug](skills/megadev-debug/SKILL.md) | Diagnose faults and stalled jobs using exact-build symbols, bounded traces and first-fault evidence. |
| [megadev-performance](skills/megadev-performance/SKILL.md) | Compare CPU, VRAM and ASIC workloads while accounting for measurement overhead and timing limits. |

### Ship your game

| Skill | What to ask it for |
| --- | --- |
| [megadev-release](skills/megadev-release/SKILL.md) | Validate disc contents and mixed-mode tracks, build portable packages and qualify a release. |

## Install

Use [Skills CLI](https://github.com/vercel-labs/skills) to choose the skills and
agents you want to install them for:

```sh
npx skills add ewrogers/megadev-skills
```

To list the collection or install just one:

```sh
npx skills add ewrogers/megadev-skills --list
npx skills add ewrogers/megadev-skills --skill megadev-project
```

Building discs needs Docker and the [Megadev image](https://github.com/ewrogers/megadev-docker).
Planning and many validation helpers work without Docker. Each installed skill
includes the references and resources it needs; related skills can be added as useful.

## Use

Ask for the task you want, and name a skill when you want to be explicit:

- "Use megadev-project to create a bootable disc in a new folder."
- "Use megadev-planning to budget an exploration game with voice and large backgrounds."
- "Use megadev-ui to build a readable pause menu and inventory for my game's style."
- "Use megadev-animation to synchronize attack poses, events and hitboxes with my game clock."
- "Use megadev-gameplay to connect movement, collisions and cameras without blocking on Sub work."
- "Use megadev-dialogue to add branching portrait conversations with optional voice."
- "Check this Main/Sub handoff with megadev-system and megadev-best-practices."
- "Use megadev-release to validate and package this mixed-mode disc."

The guidance accounts for both CPUs, RAM ownership, interrupts, the shared VDP,
CD loading, PCM/CD music and backup RAM. More disc space is not more VRAM or
unlimited background CPU time. APIs and hardware contracts are checked against
the selected Megadev source rather than borrowed from SGDK.

## Tested support

Host helpers are tested on Windows, macOS and Linux. Disc examples are built on
native Linux ARM64 and AMD64; Docker Desktop supplies the Linux environment on
other hosts. Windows Docker runtime testing is separate from host helper checks.

Build and CPU-model tests check interfaces, resources and failure handling. They
do not prove correct visuals, sound, drive timing or physical-console behavior.
Those need emulator or hardware checks. See [validation notes](docs/validation.md)
for the evidence and its limits, and the [roadmap](docs/skill-adaptation-roadmap.md)
for ownership boundaries and remaining areas.

[Harbor Relay](skills/megadev-best-practices/assets/harbor-relay/README.md) checks
scene travel, loading, audio, checkpoint saves and release packaging together.
The [Audio Lab](skills/megadev-audio/assets/audio-lab/README.md) also proves mixed-mode
release validation without adding a second audio implementation.

The skills and original examples are MIT licensed. Megadev and third-party
materials retain their own licenses and notices.

## Disclaimer

Megadev Skills is an independent community project. It is not affiliated with,
endorsed by or sponsored by SEGA CORPORATION or SEGA SAMMY HOLDINGS INC.
SEGA and related names, logos and trademarks belong to their respective owners.
They are used here to identify the hardware and tools this project supports.
No Sega BIOS files or commercial games are included.

This repository's original work is available under the [MIT License](LICENSE).
Third-party software and materials retain their own licenses and notices.

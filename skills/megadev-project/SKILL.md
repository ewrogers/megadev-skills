---
name: megadev-project
description: Create, build or troubleshoot a Megadev Sega CD/Mega CD project, including Main/Sub startup, module layouts, bootable discs and optional Mega EverDrive Pro USB deployment. Use for project foundations rather than SGDK cartridge builds.
---

# Megadev project

Make the smallest project that proves the user's requested behavior. Megadev is a lower-level kit, not SGDK with a CD attached: rendering, audio, input, resources and startup cannot use invented SGDK compatibility APIs.

## Establish the baseline

- Inspect the existing project and installed toolchain before changing its structure. Keep the user's design, controls and resource policies.
- Use `ewrogers/megadev:latest`; pull it before validation and record `doctor` output. Read the selected image's `/opt/megadev` source for API and ABI details. The references below identify the revision tested here, not a promise that `latest` never changes.
- For a new project, use [the starter workflow](references/workflow.md). `scripts/create_project.py` copies a self-contained Project Lab into a new directory; it refuses to overwrite an existing destination. Adapt or remove its diagnostic UI for the actual game.

## Preserve the machine contracts

Read [startup and ownership](references/startup.md) when touching boot code, linker layouts, C startup, callbacks or CPU communication.

- Give initialized data, zeroed BSS and stacks explicit, non-overlapping space on each CPU. Loaded bytes alone do not initialize C state.
- Keep interrupt code resident. Do not overwrite or transfer Word RAM while executing a module from it.
- Publish command arguments before the command. Acknowledgment, completion and ownership are separate events. On timeout, keep uncertain resources unavailable; do not blindly retry the same load.
- Preserve Megadev's required build flags and original-68000 target. Inspect a BIOS wrapper's register contract before calling it from optimized C; the starter uses out-of-line adapters for its BIOS calls.
- Match disc region, video configuration and emulator BIOS. Do not distribute BIOS files or change the user's emulator settings without authorization.

## Verify the result

Use [validation and troubleshooting](references/validation.md). Validate disc contents and regional boot data, then boot the CUE in an emulator. Exercise a second module load and return, not just the first screen. Report host, CPU-execution, emulator and real-hardware evidence separately.

For a user-selected Mega EverDrive Pro target, use [disc USB deployment](references/everdrive-usb.md). The bundled host Python helper stages ISO/CUE dependencies and supports build-then-deploy. Default to hard reset for each upload; soft requires an explicit user request. Updated firmware was physically verified keeping USB available in CD mode, including replacement of a running disc. The initial 4.20 setup lost USB and required menu return before upload. Check the actual setup; the stock BIOS still requires controller Start, so the whole loop is not unattended. Capture-card observation belongs to `megadev-testing`; hardware diagnostics belong to `megadev-hardware`.

The starter's UI, memory reservations, handshake and controls are teaching choices. Reuse the relevant contracts without importing its visual design or treating its small example limits as hardware limits. Advanced CD streaming, audio, graphics ASIC, backup RAM and release authoring need their own source-backed work; this skill establishes their project foundation.

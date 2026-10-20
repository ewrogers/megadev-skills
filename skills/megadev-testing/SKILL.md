---
name: megadev-testing
description: Build repeatable QA for Megadev Sega CD/Mega CD games using deterministic input replay, state checks, PAL/NTSC coverage and Main/Sub integration regressions. Supports optional EverDrive USB pad/mouse injection in cooperating development ROMs. Use for regression suites and automation; debugging diagnoses failures, while this skill establishes reproducible checks without replacing runtime service owners.
---

# Test Megadev games

Choose an observable contract and a small reproduction before building a harness. Use the selected `ewrogers/megadev:latest` sources and retain their actual compiler/SDK identity. Read [Megadev integration](references/megadev-integration.md) for the verified APIs and the limits of our test fixtures. The Lab's replay, streaming and audio helpers are application code, not SDK APIs.

## Make deterministic claims narrowly

Read [replay contracts](references/replay.md) when recording inputs, comparing simulation or changing an input/clock boundary. Reset all authoritative state, seed a game-local PRNG, sample input at a defined logical tick and record external outcomes that affect gameplay. Compare explicit state fields at each tick; report the first divergence with its input/event and expected/actual result.

Do not hash struct padding, pointers, framebuffer state or asynchronous mailbox timing into a gameplay checksum. Keep the replay schema, logical content version, initial snapshot and exact build/assets identity. Validate the entire tape before playback and use bounded recording storage. Never silently drop ticks, overwrite a full recording or continue an invalid replay as a pass.

A recorded service outcome can reproduce a gameplay decision; it does not prove that a real CD operation finishes at the same time. Deterministic fixtures and live I/O stress are separate evidence.

## Exercise integration failures

Read [integration regressions](references/integration.md) when joining CD, audio, ownership and scene services. Run the actual compiled adapters against controlled delayed/failed completions. Check their externally visible contracts: no unowned buffer access, no early publication/reuse, no music command during a live reader, safe retirement and successful later work.

Keep the game's interrupt, input, renderer, allocator and service owners. Do not install competing polling loops or introduce a test-only recovery path that hides a production bug. Include a negative control that actually fails the assertion; a test that always says PASS is not qualification.

## Check both display rates and the target CPU

Use [region and evidence matrix](references/qualification.md) for PAL/NTSC, host compilers, original-68000 execution, native ARM64/AMD64 container builds and ares. Test identical logical inputs over equal elapsed time as well as actual controller responsiveness. A PAL header or build label alone does not establish a 50 Hz emulator configuration.

[Test Lab](assets/test-lab/README.md) provides an in-memory record/replay course, a deliberate movement bug and a separate live CD/audio suite. Its [portable verifier](assets/test-lab/tools/verify_replay.py) emits JSON and useful exit codes for QA automation. It does not remotely control ares or claim automatic visual/audio verification. Read [automation](references/automation.md) for build artifacts and reporting.

For user-authorized console observation, use [direct capture-card evidence](references/console-capture.md). The bundled host Python/FFmpeg helper captures bounded video frames through macOS AVFoundation, independently of a preview window. Inspect the build ID, results and changing heartbeat/counters; keep capture success distinct from runtime verification. `megadev-project` owns EverDrive disc upload/reset. The updated firmware supports running-disc replacement; the initial 4.20 setup required menu return. The configured BIOS still needs Start, so the complete hardware loop is not unattended.

Scope seams: `megadev-debug` captures and explains failures; system/memory own lifetimes and handshakes; streaming/audio/scenes own their runtime services; graphics owns frame publication. This skill tests those contracts. Lab art, controls, music, progression and game rules illustrate a fixture and must not become requirements for the user's game.

## Emulator review

Use [ares launch and observation](references/emulator.md) to load a known build through the CLI with an explicit system and image path. Verify the active build/window before capturing or sending input; use Load menus only when the CLI cannot do the job.

## Optional USB input for development ROMs

When the user wants agent-driven real-console input, use [cooperative USB input](references/usb-input.md). The bundled portable C module and host client support two independent pad/mouse slots, gated by EverDrive EDIO detection, with bounded leases and physical fallback. Install it only into an opted-in ROM and connect it after the game's existing physical sampler; gameplay must read the resulting logical snapshot. Keep one FIFO owner, preserve SDK polling/interrupts and never claim physical-controller or BIOS-input emulation. Hardware Lab includes the hook and an input view; use direct capture plus ACKs to verify both ports, one-shot mouse movement and expiry.

---
name: megadev-best-practices
description: Apply Megadev-specific C/assembly, project-structure and integration practices while preserving CPU, interrupt and resource ownership. Use when writing or reviewing Megadev game code, joining existing systems, or fixing cross-system correctness problems; detailed graphics, audio, loading, save and release implementations remain with their own skills.
---

# Megadev best practices

Adapt the smallest useful pattern to the user's game. These are implementation
practices, not an art direction, control scheme, genre or required engine layout.
Megadev helpers, BIOS entry points and this example's application functions are
separate APIs; do not substitute SGDK APIs or teach the latter as SDK facilities.

## Preserve the owning system

Before joining features, name the owner of each clock, interrupt hook, mailbox
word, RAM region, buffer, drive operation, voice and save namespace. Reuse that
owner's request/completion contract instead of introducing a second manager.
Read [integration seams](references/integration.md) for the concrete ownership
map, generation/lease rules and safe handling of late results.

Follow the project's structure. A useful seam separates portable policy from
platform adapters, exposes what the caller needs, and hides the fragile handshake.
Do not create a generic framework or split tiny modules merely to match the Lab.
Read [structure and review](references/structure.md) for choosing that boundary.

## Write C and assembly for the selected target

Read [C/ABI practices](references/c-and-abi.md) for promotions, overflow, alignment,
byte decoding, aliasing, stack and inline-assembly obligations. Use the selected
`ewrogers/megadev:latest` source/compiler to verify an API or ABI; retain original
68000 and compiler safeguards. Limit Docker build containers to `--cpus=2`.

Do not use `volatile` as an ownership, atomicity or compiler-barrier substitute.
Publish complete immutable payloads, then their commit word; hold storage until
acknowledgement. Keep critical sections short, preserve the caller's interrupt
policy, and leave BIOS/file/save work outside interrupt callbacks. The example's
Main publisher is called only from its interrupt-enabled foreground loop.

## Prove integration behavior

Use [Harbor Relay](assets/harbor-relay/README.md) when a runnable cross-system case
is useful. It reuses the established scene, CD/audio and save implementations,
with an explicit checkpoint adapter and small executable C contract examples.
Create it using `scripts/create_project.py`; it works after standalone installation.

Read [validation](references/testing.md) for O0/O3 checks, original-68000 execution,
ARM64/AMD64, PAL/NTSC, release qualification and emulator/hardware limits. Test busy,
failed, cancelled and late operations as well as the successful route. A timeout
must not make live storage reusable. Fix the owning system when a real shared bug
is found; do not hide it in a second local workaround or weaken its checks.

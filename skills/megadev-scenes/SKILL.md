---
name: megadev-scenes
description: Build Megadev Sega CD game flow, scene transitions and resource cleanup, including fades, preloads, pause states, failed-load recovery and safe Main/Sub retirement.
---

# Megadev scenes

Adapt the game's own flow and resource lifetimes. A scene is a project concept;
it is not automatically a Megadev module, a Sub program or a new allocation pool.
Keep the user's setting, controls, presentation and progression independent of
the Lab's choices.

- Read [Game flow](references/flow.md) for states, pause, transitions, rollback
  and choosing resident code versus loadable modules.
- Read [Ownership and cleanup](references/cleanup.md) before releasing assets,
  changing interrupt callbacks or retiring work across Main/Sub.
- Read [Validation](references/testing.md) for failure cases, executed tests,
  regional timing and the emulator/hardware boundary.

Essential contracts:

- One flow owner accepts scene requests. Define what another request means
  during a transition; do not restart the loader or recursively enter a scene.
- A gameplay pause does not stop the CD access service, mailbox retirement,
  audio control or the VBlank publisher. Use the game's existing clocks.
- Fade out, confirm the displayed palette is black, retire old work, prepare
  and validate the next resources, publish the new view, then fade in.
- Cancellation requests do not prove completion. Keep buffers, code and callback
  targets alive until the actual reader/worker and presentation consumer retire.
- Keep a recoverable old view until the replacement can commit. A timeout may
  report a stall; it must not manufacture an ownership grant or free live data.
- Keep shared art, fonts, audio and the Main/Sub service resident where useful.
  Reclaim scene-local state deliberately; bound retained caches separately.

Use `ewrogers/megadev:latest`; inspect that image's source before teaching an API.
The [standalone Transition Lab](assets/transition-lab/README.md) combines a title,
options, two areas, pause, results and credits. Its `scene_*`, `stream_*` and
`audio_*` calls are example project code, not Megadev APIs. Containers use at most
two CPUs.

Reuse the game's existing system, memory, streaming, graphics and audio owners.
This skill composes their lifetimes; it does not introduce another CD reader,
allocator, input driver, renderer or music driver. Detailed source links and
audited revisions are in the references.

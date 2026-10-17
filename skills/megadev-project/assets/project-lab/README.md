# Project Lab

A small Megadev disc demonstrating Main/Sub startup, a bounded runtime command handshake and a reloadable Main module. It is original teaching code with bootstrap portions adapted from Megadev's MIT-licensed `examples/new_project`; see `LICENSE.megadev`.

Generate a copy with the skill's `scripts/create_project.py` rather than building this template directly. Its Makefile contains metadata placeholders. From the generated project directory:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue project_lab.iso
```

Replace `project_lab` with the ID supplied to the generator. In PowerShell use `source=$($PWD.Path)` in the mount argument. Launch the resulting CUE with `ares --system "Mega CD" /path/to/project_lab.cue`, using an already configured, matching BIOS. On macOS, use `open -a /Applications/ares.app --args --system "Mega CD" /path/to/project_lab.cue`.

For EU/PAL, clean then build with `REGION=EU VIDEO=PAL`; for Japan use `REGION=JP VIDEO=NTSC`. Default is US/NTSC. Build to a separate directory and reload a fresh disc rather than replacing a running ISO. Full workflow details are in the installed skill's `references/workflow.md`.

## Try it

Boot with your configured Start button. The home screen reports three startup checks and two live counters.

- D-pad: move the marker.
- Start: hold/resume the marker; CPU services keep running.
- B: ping Sub and show its reply.
- A: load the room from disc. Its initialized data and zeroed RAM should pass.
- In the room, A or Start returns. Loading it again proves its private state resets.

All numeric counters are hexadecimal. The initial reply may show 0064 from the bootstrap's successful CD operation before the first ping. A load timeout or failure asks for a reset; it deliberately does not retry with uncertain ownership.

This diagnostic screen and its memory layout are examples, not a required game architecture. It does not demonstrate audio, artwork, saves or advanced streaming.

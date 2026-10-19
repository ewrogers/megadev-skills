# Harbor Relay

A small cross-system Megadev example: two areas, music/PCM, preloading, fades and
an explicit checkpoint. It demonstrates integration contracts; its art, controls,
progression and record fields are illustrative choices for this example.

## Build

```sh
python3 /path/to/megadev-best-practices/scripts/create_project.py ./harbor-relay
cd harbor-relay
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build REGION=US VIDEO=NTSC
```

Build fresh folders with `REGION=EU VIDEO=PAL` for the PAL case. Region/security
and video timing are separate build choices. Open `harbor.cue`, keeping its ISO
and `res/track02.wav` / `res/track03.wav` together. Ares requires a separately
configured BIOS; launch explicitly with its Mega CD system. Do not distribute BIOS files.

## Controls

- Title: Up/Down selects Start / Continue / Options / Credits; A or Start selects.
- Fly with the D-pad. Reach the white-and-gold beacon on the right, then A travels.
- B preloads the next area; C returns to title and safely drains outstanding work.
- Start pauses. While paused, B saves a checkpoint and C returns to title.
- Options changes flight speed with Left/Right; B returns.
- C on title intentionally requests a missing area and recovers to title.

The footer shows checkpoint status. Continue restores the saved area, horizontal
position, flight speed and beacon progress. Saving is explicit and uses only the
Lab's `MDBESTLAB_A/B` internal backup-RAM files. It never formats, deletes, damages
or accesses another game's files. If media is unavailable/unformatted, it reports
that condition. A timed-out save keeps ownership reserved and asks for reset.

## Small executable C examples

With a host C17 compiler installed:

```sh
python3 tools/check_contracts.py
```

This checks bounded slices, byte decoding and clock conversion at O0/O3. It does
not measure 68000 speed. Repository tests also execute the compiled Main/Sub
checkpoint and scene glue, with BIOS/MMIO models and native ARM64/AMD64 builds.

`TOUR=1` runs the scene/load recovery tour without writing backup RAM. It does not
automatically perform the manual checkpoint test.

## Release proof

`tools/release.py` is the unchanged release-owner helper. Create an approval
contract using `harbor.cue`, `disc/` and `build/`, record actual source/SDK/compiler
provenance, then package and verify the mixed-mode disc. See the [release workflow](https://github.com/ewrogers/megadev-skills/blob/main/skills/megadev-release/references/workflow.md) for the supported profile and compatibility limits. The helper is bundled; no other skill installation is needed. Reopen a fresh
unpack for acceptance; do not rely on files in the developer build directory.

Existing streaming/audio/scene policy and adapters remain unchanged. The new
checkpoint adapter reserves an existing mailbox command value after startup;
it adds no second reader, audio manager, renderer or interrupt handler.
Art and music are reused from our original Labs; the upstream starter notice is
in `LICENSE.megadev`. The repository's own code/documentation are MIT licensed.

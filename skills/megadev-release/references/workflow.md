# Build, approve and package

The helper is Python standard library only and works from outside the project.
Commands below use a POSIX shell; on PowerShell use `$PWD.Path` for the bind mount
and one line per command. The Python commands and relative package layout are the
same on all three host OSes.

## Existing mixed-mode proof case

Use the existing Audio Lab from `megadev-audio`, if installed. Do not create a
second release-specific runtime. Its own generator, Makefile and CUE tool remain
the owners of the sample disc. A normal game can use the same release helper
without installing that example skill.

```sh
python3 /path/to/megadev-audio/scripts/create_project.py ./audio-release
cd audio-release
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest doctor
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build REGION=US VIDEO=NTSC
python3 tools/make_cue.py audio_lab.iso
```

Use `--platform linux/arm64` or `linux/amd64` for a deliberate architecture check;
use native runners for CI evidence. Keep `latest` in normal commands, but retain
the actual image identity/digest and the full doctor output with private build
records. Avoid silently changing compiler/SDK between approval and packaging.

## Approval contract

```sh
python3 /path/to/megadev-release/scripts/release.py inspect audio_lab.cue --region US
python3 /path/to/megadev-release/scripts/release.py contract audio_lab.cue --region US --video NTSC --payloads disc --boot build --output approved-disc.json
python3 /path/to/megadev-release/scripts/release.py validate audio_lab.cue --contract approved-disc.json
```

The contract binds the data track to every file in `disc/`, the linked boot binary
bytes, track order, indices, file hashes and decoded PCM hashes. Generate it from
a trusted candidate and retain it. Review the file list and tracks before treating
it as approved; do not generate it from an arbitrary downloaded disc to certify
that download. A structurally valid package can still contain a broken game.

Create a `provenance.json` file with these four text fields:

| Field | Value |
| --- | --- |
| `source_commit` | Full 40-character commit for the game, including release fixes. |
| `sdk_commit` | Full Megadev commit printed by `doctor`. |
| `compiler` | Actual target compiler/version, such as `m68k-elf-gcc 16.2.0`. |
| `container_reference` | Image used; record its resolved identity in build evidence too. |

Do not claim a commit reproduces dirty source. Commit changes or separately
preserve and identify the source patch before making the release contract.
These fields are recorded declarations, not independently authenticated attestations.

## Portable archive

```sh
python3 /path/to/megadev-release/scripts/release.py package audio_lab.cue --contract approved-disc.json --provenance provenance.json --include README.md --include LICENSE.megadev --output audio-release-us.zip
python3 /path/to/megadev-release/scripts/release.py verify audio-release-us.zip
```

Add your own license and asset/music notices explicitly with further `--include`
paths, relative to the CUE directory. Only referenced disc files and explicitly
included files are copied. Generated contracts/archives use exclusive output:
choose a fresh path rather than overwriting an approved candidate.

The embedded manifest contains the approved contract, provenance and SHA-256/size
for every delivered file. The ZIP is deterministic for identical inputs. Fresh
ISO builds may differ in volume/directory timestamps; CI compares boot, module,
asset and audio payloads and separately validates each exact packaged ISO. It
**does not claim whole-ISO reproducibility**. Do not broadly zero metadata or ignore
unknown differences merely to produce matching hashes. A future reproducible
mastering process needs its own verified timestamp policy.

Unpack into a fresh folder and open the delivered CUE. Test away from the build
folder so missing relative tracks/files cannot accidentally resolve. Never ship a
BIOS, emulator configuration/save, developer build tree or commercial reference disc.

# Create and build

Requires Python 3.10+ and Docker running Linux containers. The public toolchain is [ewrogers/megadev](https://github.com/ewrogers/megadev-docker); native images support ARM64 and AMD64. Docker Desktop supplies Linux containers on macOS and Windows.

Run the generator from the installed skill directory:

```sh
python3 scripts/create_project.py /path/to/my-game --id my_game --title "My First Game"
```

Use `python` if that is your Python command. The ID is a short lowercase build filename, not the on-screen title. The title sets disc metadata; edit the Lab's diagnostic UI separately. The destination must not exist. No host Megadev installation is needed.

From the generated project directory, POSIX shell:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 ewrogers/megadev:latest doctor
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest cue my_game.iso
```

PowerShell uses `source=$($PWD.Path)` in the mount argument. The Windows command is otherwise the same; use Linux containers, not Windows-container mode. On Linux, optionally add `--user "$(id -u):$(id -g)"` to avoid root-owned outputs. Paths containing spaces work when the entire mount argument is quoted.

Builds default to US/NTSC. For another target, clean before changing configuration:

```sh
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build clean
docker run --rm --cpus=2 --mount "type=bind,source=$(pwd),target=/workspace" ewrogers/megadev:latest build REGION=EU VIDEO=PAL
```

Use JP/NTSC for Japan. The Makefile's `OPT_FLAGS` supports a Lab debug build, e.g. `OPT_FLAGS=-Og -g -fconserve-stack -fomit-frame-pointer` plus `DEBUG=1`, each passed as one argument where needed. Do not set `CC_FLAGS` on make's command line: doing so can suppress framework `+=` flags. Other projects may expose different override variables; inspect their Makefiles.

## Outputs and boot

- `my_game.iso`: data-only disc image, not a cartridge ROM.
- `my_game.cue`: track description referencing that ISO.
- `disc/`: resident Main MMD, Sub SMD and reloadable room MMD.
- `build/`: ELF, symbols and intermediate bootstrap data useful for debugging.

Ares on macOS:

```sh
open -a /Applications/ares.app --args --system "Mega CD" /absolute/path/to/my_game.cue
```

On platforms with `ares` on PATH, use `ares --system "Mega CD" /path/to/my_game.cue`. Quit an idle existing instance if the launcher does not pass the new arguments through. Select a matching BIOS already supplied by the user. At the BIOS screen, press the configured Start button.

Mega CD controller bindings can differ from Mega Drive bindings. Check the active system's mapping if Start works but A does not. J/K/L are only common local mappings, not an emulator guarantee.

Keep a loaded disc immutable. Build in a separate directory, then reload the whole disc. Replacing an ISO while old resident code remains running can mix module exports from different builds.

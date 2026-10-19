# Language Lab — The Postcard Desk

A small four-locale UI demonstrates English, French, German and Spanish text,
native accents, conversation paging, bounded name entry, inventory quantities
and an honest font budget. Art, wording, font and controls are illustrative.
The Lab is intentionally silent. It does not install a voice/drive/save system.

## Build and open

From the installed skill:

```sh
python scripts/create_project.py language-game
```

In that new folder:

```sh
docker pull ewrogers/megadev:latest
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest build
docker run --rm --cpus=2 --mount "type=bind,source=$PWD,target=/workspace" ewrogers/megadev:latest cue language.iso
```

Use your chosen ID if different. Keep CUE beside ISO. For a PAL build, generate a
fresh folder and append `REGION=EU VIDEO=PAL` to `build`. Use an absolute host
mount path on Windows; choose `--platform linux/arm64` or `linux/amd64` as needed.
Unix users can add `--user "$(id -u):$(id -g)"` for host-owned outputs.

On macOS with a personally supplied configured BIOS:

```sh
open -a ares --args --system 'Mega CD' "$PWD/language.cue"
```

## Try it

- Home: Left/Right changes language; Up/Down selects; A opens.
- Conversation: A pages/returns. The diamond and page count signal more text.
- Inventory: A cycles 0, 1, 2, 999 and 65535 to test complete translated sentences.
- Name: D-pad selects letters/accents/digits; A adds, B erases. DEL/OK are grid
  actions. Names hold up to eight glyphs. B on an empty name returns home.
- Font Budget shows used glyphs/pattern bytes, reserved tiles and free slots.
- C changes language inside any normal screen. B returns, retaining home focus.
- Start pauses/resumes. Typical ares bindings are J/K/L for A/B/C, Enter for Start;
  check the actual Mega CD profile rather than assuming keyboard mappings.

## Authoring and budgets

Edit UTF-8 `res/catalog.json`. Explicit message IDs and locale keys separate
wording from logic. `tools/catalog.py` normalizes NFC and rejects missing text,
placeholder mismatches, unsupported glyphs/controls and budget/layout errors.
The generated C catalog and native font are rebuilt together. The compact
runtime codec is application code, not Megadev/BIOS text encoding.

The shared Latin font uses 117 glyphs (3,744 pattern bytes) within a 192-tile
reservation (6,144 bytes). It is not a full Unicode font. The padded atlas and
catalog also occupy resident Main Work RAM in this standard CD-boot example.
The renderer stages complete language pages in a hidden Plane A table, then
flips after acknowledgement. It updates at most four 40-cell rows per VBlank;
there are no steady-state font or palette uploads.

Run `python tools/check_language.py` with a host C17 compiler for portable O0/O3
formatting, layout, navigation and publication checks. `TOUR=1` builds an automatic
screen/language tour. Native architecture/regional matrices and original-68000
execution are in the repository's tests. Emulator tests and physical-console
video/timing qualification are separate; see the skill's validation reference.

Original glyph art is adapted from the MIT-licensed SGDK Language Lab's portable
font patterns; no SGDK runtime or API is used. Bootstrap code derives from
Megadev; retain `LICENSE.megadev` and the project MIT notice.

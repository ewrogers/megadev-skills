---
name: megadev-localization
description: Prepare Megadev Sega CD games for translated text with stable message IDs, accented glyphs, font budgets, safe substitutions and translated layout checks. Use for multilingual UI, dialogue and language preferences rather than general text rendering alone.
---

# Megadev localization

Start with the game's existing catalogs, languages, fonts and renderer. Preserve
its creative direction. Language Lab's harbor desk, portrait, colors, font,
controls and wording illustrate techniques; they are not defaults for a game.

- Read [Catalogs and encoding](references/catalogs.md) for message/language IDs,
  UTF-8 authoring, normalization, substitutions, plurals and fallback.
- Read [Fonts and memory](references/fonts.md) for native accents, atlas/glyph
  budgets and the additional Main RAM cost of resident Mega CD resources.
- Read [Layout and validation](references/testing.md) for translated expansion,
  paging, name/number limits, atomic language changes and evidence boundaries.

Megadev exposes VDP and BIOS facilities, not a Unicode decoder or localization
framework. Inspect the selected `ewrogers/megadev:latest` source before using an
API. Do not copy SGDK text/resource functions or assume BIOS printing performs
UTF-8 decoding, shaping, glyph fallback or wrapping.

Essential contracts:

- Separate stable message keys from translated prose and game logic. Make locale
  identifiers explicit; validate unknown preferences before indexing catalogs.
- Validate the required character repertoire before export. Normalize authoring
  consistently, count glyphs/advances rather than UTF-8 bytes and reject missing
  glyphs. Never silently strip accents or turn unsupported text into random tiles.
- Substitute bounded named values into whole translated messages. Check that
  locales agree on placeholders; never use translated text as a printf format.
  Use explicit language-appropriate plural rules, not English word fragments.
- Budget source/catalog bytes, Main RAM residency, temporary layout storage,
  font patterns, tilemaps, palettes and transfers separately. Disc capacity does
  not make fonts free in Main RAM or VRAM.
- Validate every real text box with the longest supported names, quantities and
  variants. Include choices, tooltips, name-entry keys and error messages. A
  renderer that clips silently is not a translated-layout validator.
- Reuse the existing renderer/input/clock owners. Keep old fonts/text alive until
  their page or bank is retired; publish a language change only when its complete
  layout and matching glyph resources are ready. Clear stale longer text.

[Language Lab](assets/language-lab/README.md) is a standalone four-locale Latin
example. Its offline compiler, native glyph patterns, portable model and safe
page publisher work after installation without another skill. The `lc_*` and
`ui_*` helpers are application code, not SDK APIs or a universal Unicode engine.

Use `megadev-ui` for general presentation/navigation, `megadev-dialogue` for
conversation progression, `megadev-assets` for asset preparation,
`megadev-streaming`/`megadev-memory` for bank lifetimes and `megadev-save` for
versioned preferences. Localization supplies text/repertoire/layout contracts
without creating rival drive, input, DMA, save or audio services.

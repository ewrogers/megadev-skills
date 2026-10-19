# Translated layouts and validation

## Check the actual boxes

Measure rendered glyphs/advances, not source UTF-8 bytes. Test all locales using
maximum supported names and quantities, every plural variant, explicit line
breaks and long words. Include prompts, error text, small keyboard buttons and
menu labels. Decide whether a long word wraps, breaks, shrinks or fails; do not
silently clip translations. Language Lab breaks an overlong word across lines
without inventing hyphens and rejects pages beyond its bounded capacity.

The offline compiler validates one-line source bounds; the executable UI checks
its actual narrower fields and wrapping regions. These are complementary tests.
The Lab's 24-column five-line conversation, two-line menu labels and 34-column
footer are examples, not a preferred layout for every language/game.

Keep focus when backing out of a translated menu. On an in-place locale change,
reset or remap a dialogue page that no longer exists. Names should survive a
language switch when their glyphs are supported. Test empty/full names, accent
selection, erase/confirm, and switching from a long string to a shorter one.

## Publish a complete translation

Reuse the renderer, input snapshot and 300 Hz game/UI clock. The Lab keeps two
Plane A name tables. It stages four rows per VBlank and flips only after the
complete hidden page arrives. A locale change is part of the page key; matching
text/font resources remain resident. Ordinary focus/name changes use bounded
dirty rows. Pending batches stay immutable until acknowledged. Late or active-
display callbacks defer without consuming work; invalid batches fail closed.

The example has no active-display CRAM updates and installs no competing HBlank,
CD, audio or save owner. BIOS Sub INT2 service remains alive. Pausing is an
illustrative UI policy; it freezes navigation/reveal state while the publication
owner can finish its current transfer.

## Meaningful automated checks

- `tests/test_localization.py` checks NFC equivalence, explicit IDs despite JSON
  object reordering, placeholder/glyph/control/layout failures, native glyph
  pixels, exact substitutions, wrapping against an independent text oracle,
  all translated screens with eight-glyph names and 65535 quantities, timing,
  focus/name bounds and standalone creation.
- `tools/check_language.py` is self-contained after installation. It compiles
  the portable C catalog/layout/model/publisher contracts at O0/O3 and compares
  checksums. No SGDK library or Megadev BIOS is needed for these host checks.
- `tests/localization_cpu.py` executes the real original-68000 functions. It
  compares every formatted message to UTF-8 source, exercises layout/model
  boundaries and stack guards, and captures exact immutable VBlank rows and
  final page flips. It rejects late/active/malformed publication and stale work.
  Tour execution must actually visit all screens/locales.
- The incremental build regression changes a translation to require a new
  accented glyph, rebuilds without cleaning, then executes the changed formatter
  and compares embedded font bytes. Restoring the source must reproduce the
  original module. A source-only compiler test cannot catch stale C/incbin output.
- Native ARM64/AMD64 CI builds US/JP/EU release, US debug and US tour. Payload
  comparison requires a complete matching matrix and executed localization,
  publication and incremental evidence. JP here is a regional build, not a claim
  that the example supports Japanese text. CI installation disables telemetry.

## Ares acceptance and limits

Load the CUE explicitly as Mega CD with your own configured regional BIOS.
Check language selection, all accented forms, conversation paging and its more-
text indicator, zero/one/many quantities, the name grid, restored menu focus,
font counts, in-screen locale changes, short-after-long text and pause. Repeat
in PAL and NTSC, looking for partial pages, stale glyphs, flicker or CRAM dots.
The Lab is intentionally silent; it tests text/layout rather than voice/audio.

Host checks and CPU/MMIO models do not establish video quality, hardware bus
or VBlank timing. Physical consoles still need display/overscan, font contrast,
DMA/publication and combined CD/audio/ASIC qualification. Translation wording
and reading comfort need native-speaker review; the sample prose is illustrative,
not a professionally reviewed localization pack.

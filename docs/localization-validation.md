# Localization validation

Language Lab demonstrates four Latin-script locales through the same renderer:
English, French, German and Spanish. It is intentionally silent. Its harbor desk,
font and prose illustrate contracts rather than prescribing presentation.

## Contracts exercised

The offline compiler rejects missing/empty translations, duplicate JSON keys,
unknown or mismatched substitutions, unsupported glyphs, malformed controls,
font overflow and layout overflow. NFC normalization and explicit message IDs
make equivalent source text and reordered objects deterministic. Runtime tests
compare every substituted message to the readable source, including eight-glyph
names and unsigned 16-bit quantities. An independent wrapping oracle checks
explicit newlines, narrow fields and overlong words.

The atlas uses 117 native 8×8 glyphs (3,744 bytes) in a 192-tile reservation
(6,144 bytes). Source patterns/catalogs also occupy Main Work RAM; they are not
free ROM-resident resources in this CD-boot layout. Layout temporaries have
bounded stack use, tested against a guard below the Main stack. The live model,
canvas, publisher and immutable batch have separate bounds.

Original-68000 execution checks the compiled formatter, wrapping, navigation,
PAL/NTSC repeat behavior, full-name limits and all translated/paused screens.
MMIO capture checks four-row publication, exact words, hidden-page flips,
late/active-display deferral and malformed batches. Automatic tours must commit
all five screens and all four locales. An incremental build adds an accented
glyph without cleaning: both compiled text and embedded font must change, then
restoring the source must reproduce the baseline module.

## Build matrix and evidence

Each architecture builds US/JP/EU release, US debug and US automatic tour with
`ewrogers/megadev:latest`, two CPUs per container. Local validation runs ARM64
natively and AMD64 through Docker emulation; CI uses native runners for each.
Cross-architecture comparison checks complete matching target payloads and
requires execution markers, not merely successful compilation. JP identifies
the disc region, not Japanese-language support.

The image inspected for this milestone uses Megadev
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` and GCC 16.2.0. Reports record the
actual image/tool revisions; public commands follow latest.

NTSC and PAL user testing confirmed navigation, accents, paging, name entry
and the corrected single selection highlight. The initial NTSC check caught
extra highlights on the language label and unused second menu row. Rendering now leaves
fully blank rows as panel background; tests check both the empty English row
and the genuinely wrapped French label. Name-grid and budget labels are also
checked against the panel's bottom border.

Native CI status is recorded in the PR. Real
hardware remains unqualified: check overscan, readability, VBlank/bus timing and
combined CD/audio/ASIC activity. Sample translations need native-speaker review.
The private glyph codec is not universal Unicode or a stable save-name format.

## Ownership

Localization supplies catalog, repertoire and layout contracts. UI owns panels
and publication, dialogue owns conversation progression, input owns devices,
streaming/memory own residency and saves own versioned preferences. The example
keeps the existing BIOS/Sub servicing and adds no rival interrupt, DMA, drive,
audio or save system.

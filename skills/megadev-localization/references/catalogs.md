# Catalogs, encoding and wording

## Separate authoring from rendering

Keep readable UTF-8 source text and stable semantic message keys. The Lab uses
`res/catalog.json`: an explicit ordered `ids` list, locale keys and one translation
per locale for each message. Translation-object ordering cannot change generated
IDs; append new IDs deliberately and do not recycle existing ones. Generated
numeric IDs are a build contract, not a universal save-file or network format.

`tools/catalog.py` normalizes text to NFC, checks duplicate JSON keys, missing
translations, malformed/unknown placeholders, placeholder agreement, control
characters, authored glyph coverage and reservation/offset limits. It emits
`src/catalog.c`, `src/catalog.h` and `res/catalog-report.json` deterministically.
Generated files use explicit UTF-8/LF on every host. Edit the source catalog,
not those generated files. A failed validation must stop the build.

The example codec uses glyph codes 1..N, zero termination, 252 for newline and
253/254 for name/count substitution. This is a private application format.
The runtime formats those tokens into glyphs, then the layout engine measures
and wraps them. UTF-8 bytes are never passed directly to tile selection.
The catalog's 16-bit offsets impose a 64 KiB text-blob bound; lookup tables
add to the complete bank size. This is an example format limit, not a Mega CD text-capacity limit.

Choose a codec for the project: UTF-8 decoding, stable character IDs, compact
per-bank glyph IDs or another validated representation. A Latin subset and
Arabic shaping need different renderers. Do not describe this example's compact
byte codec as complete Unicode support.

## Substitutions and plural forms

Use complete translated messages such as `You have {count} apples.` with named,
bounded values. The compiler checks the same placeholder multiset across all
locales, including repetitions. Literal percent signs remain literal text;
translations are never printf format strings. The Lab bounds names to eight
glyphs and quantities to unsigned 16-bit values. Formatting sizes the entire
result first and leaves the destination unchanged on invalid input or overflow.

The sample has separate zero/one/many sentences for its English, French, German
and Spanish inventory examples. `lc_variant` selects those specific authored
sentences. It is not a universal plural engine: grammatical categories, gender,
cases, number formatting and word order must follow the chosen languages.
Avoid adding English suffixes or concatenating word fragments around a number.

## Preferences and missing content

Use stable locale keys in persistent preferences, or version a numeric mapping.
`lc_language` demonstrates a checked unknown-ID fallback to the default locale;
validate it before storing into the UI model. Campaign saves remain the save
owner's responsibility. Keep the default language usable before a preference
is loaded, and choose an intentional policy for missing localized files.

The Lab's compiler rejects empty or missing translations rather than hiding them through
fallback. A project may allow explicit fallback, but report it during validation
and define whether dialogue text/voice can differ. Never treat an accidentally
missing entry as a valid empty string.

Private glyph indices can change when a font repertoire changes. Do not persist
the Lab's raw name bytes across unrelated font revisions without a versioned
mapping. Save stable character IDs or Unicode text and validate/convert it when
loading. The Lab does not write preferences or names to backup RAM.

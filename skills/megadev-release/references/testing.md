# Release qualification

## Automated evidence

Host tests exercise valid mixed-mode packaging and byte-identical repeat archives,
plus missing/changed audio, bad WAV format, partial sectors, bad indices, path
traversal/case aliases, symlinks, regional/security damage, malformed ISO numbers,
out-of-range/overlapping extents and damaged/incomplete archives. A synthetic
security fixture tests parser decisions; it is not a BIOS acceptance check.

The real proof case is the **unchanged Audio Lab**. Its native ARM64 and AMD64
integration jobs build US/JP NTSC, EU PAL, debug and tour variants. Each one:

1. Retains the existing Main/Sub startup, original-68000 callback, CD reader and
   audio-adapter regressions.
2. Checks the actual SDK boot/security blocks and all compiled disc payloads.
3. Packages the ISO/CUE/two WAV tracks with selected instructions/notices, then
   verifies that exact archive and reproduces its bytes from identical inputs.
4. Rejects actual missing/mutated tracks, changed pregaps, wrong paths, damaged
   security code and a damaged ZIP. Inputs are restored and revalidated afterward.

Reports must contain that evidence; architecture comparison rejects its absence.
Boot/modules/artwork/audio and track semantics must match. ISO creation metadata
and whole ZIP hashes from separate builds are not presented as matching evidence.
CI installs all skills with telemetry disabled and checks the standalone helper.
No additional target runtime or rival CD/audio implementation is introduced.

## Emulator acceptance

Use a fresh unpack of the packaged artifact and explicitly select Mega CD when
launching ares. A separately configured, legally supplied BIOS is required; do not
copy it into the release. For Audio Lab:

- On CD MUSIC, A plays, B pauses/resumes, C stops; Up/Down selects both tracks.
- Left/Right changes pages. On PCM DESK, A auditions, B changes pan and C toggles
  the looping bed. Verify these are heard with the expected levels/channels.
- On the load/handoff page, exercise the documented load and return sequence.
  Check text/art, audio restart and repeated visits rather than boot alone.
- Repeat the relevant checks on explicitly selected PAL/NTSC builds. A working
  NTSC disc does not certify PAL or every BIOS. Keep the video rate, disc region,
  emulator version and BIOS/model identity in the result where known.

Ares confirms that its implementation boots/plays the package in that configuration.
It does not measure real seek/drive timing, analog PCM quality or burned-media quality.

## Physical-console acceptance

Record unmodified console/region, BIOS revision, reader/drive or development media,
controller setup and the exact package digest. Qualify cold boot and soft return,
all data areas/music tracks, loading while music/PCM is active, cancellation and
repeated transitions. Include slow/worn-drive behavior, long sessions and recovery
paths relevant to the game. Keep existing saves intact; never make formatting a
release smoke test. Disc-based and optical-drive-emulator tests are distinct evidence.

Do not promise all models/regions based on one setup. Document unsupported cases,
known issues and tested controls in the delivered instructions. Publication,
mastering/burning and distribution require the user's requested release scope.

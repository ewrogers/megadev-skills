# Megadev integration and budgets

Verified against `ewrogers/megadev:latest`, Megadev revision
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`, GCC 16.2.0. Inspect the selected
image's source and record its actual revision when adapting another version.
Public commands use `latest`, with Docker build/test containers limited to two CPUs.

## Verified platform boundary

The selected SDK's `lib/main/io.def.h` defines BIOS-format SACBRLDU masks, not
SGDK's masks. `lib/main/bios.h` supplies `bios_joy1_hold` and `bios_joy1_hit`.
`lib/main/bios.def.h` defines Window A000, Plane A C000, Plane B E000, SAT B800
and HScroll BC00. The standalone Lab retains the gameplay Lab's C ABI/IRQ bridge
and conservative `gfx_sprite` admission implementation unchanged.

The project IRQ preserves registers, triggers Sub INT2 and BIOS pad sampling,
then consumes an immutable sprite/HUD snapshot. It does not run the timeline,
collision or sound callbacks. Main foreground does those jobs. This installs a
frame/input owner only in the standalone Lab; an existing game keeps its owners.

The [Megadev source](https://github.com/drojaazu/megadev) and selected image's
headers/startup assembly are authoritative. `an_*`, `tr_*`, `gfx_*` and `lab_*`
are project functions. Do not invent `SPR_*`, `TSK_*` or SGDK resource APIs here.

## Residency and transfer

Every animation pose is resident in Main RAM/VRAM. Frame selection changes SAT
pattern indices, with no per-frame allocation, CD load or Sub transaction.
Sub only performs startup loading and continues BIOS access servicing; it is not
an animation timer pool. A large cast may need shared pose residency or measured
streaming, but that is a separate resource policy, not automatic offloading.

Keep a streamed frame's graphics valid until no published/displayed SAT can
reference them. Logical clip completion is not proof that the renderer released
its frame or that an outstanding read stopped. Use scene/streaming leases and
safe retirement. Never use a frame event to grab Sub-owned Word RAM directly.

Two 1640-byte Main snapshots hold SAT admission data and four HUD rows. One
aligned word publishes a completed snapshot; a pending one cannot be overwritten.
The early-blank consumer defers late service and uploads at most 448 bytes for
16 project sprites plus the HUD. Scanline admission separately enforces H40
hardware limits. Normal frames do not write CRAM; initial art/map/palette setup
happens with display disabled. Byte limits are not proof of real raster margin.

The 48-byte controller contains a 10-byte playback state. Frame tables are
immutable and stored with Main code/art. Native art includes 190 sprite tiles,
96 font tiles and ten ground tiles, four palette lines, and no PCM/sample pool.
Pattern indices are font 0020, actors 0080, ground 0180; addressed pattern space
ends at 018A (12608 bytes including gaps), below the BIOS tables. Main linker
regions reserve code/art FF0000..FF9FFF, data/BSS FFA000..FFC3FF and stack
FFD000..FFDFFF, leaving BIOS storage untouched. Recheck linked high-water marks
when adding art or actors rather than treating these sample reservations as an
engine requirement.

The Lab's small original PSG tune/cues have one Main-only owner and no Z80 sound
driver. The port/command format follows [PSG hardware documentation](https://plutiedev.com/psg).
An existing game routes footstep/swish/hit events through its audio service;
it must not add another direct PSG writer. Pause and music behavior remain the
user's design decision, independent of how this inspection Lab sounds.

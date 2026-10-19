# Validation and qualification

## Behavior and original-68000 execution

`tests/test_raster.py` checks PAL/NTSC simulation equivalence, pause/input policy,
waterline bounds and exact reflection coordinates, independent split cameras,
SAT links and every scanline's sprite-entry/pixel budget. It reconstructs native
art, validates VDP colors and reflected flip/index mapping, exercises invalid
plans and standalone creation, and compares long O0/O3 runs with UBSan where
supported. The C host library has explicit Windows exports and unload cleanup.

`tests/raster_cpu.py` executes the compiled original-68000 model, publication,
VBlank consumer, vector installation/detach and direct H handler. It verifies:

- Full RAM vector and Gate Array low-word preservation and idempotence.
- Immutable pending snapshots and validated plan copies.
- Exact BIOS DMA destinations, byte sources, word counts and C ABI clobbers.
- Dry restore/counter programming before bulk work, bounded payloads and arm
  only after safe setup. Late setup is rejected and the following safe frame
  can recover.
- H entry/exit register and stack preservation, an RTE return, current epoch,
  fresh edge waits, table/VSRAM values and the tiny optional CRAM pair.
- Pending VBlank/blank-edge/early requests leave the new arm intact; late
  requests and stuck-high/stuck-low HBlank flags terminate without unsafe writes.
- A new epoch between batches prevents an old handler's palette write and does
  not clear the new arm.

VDP flags, scanlines and BIOS DMA are modeled in these tests. This proves code
contracts and failure handling, not FIFO timing, bus cycles or CRAM-dot absence.
The real moving-water bug required an emulator sweep; preserve both kinds of
checks instead of treating instruction-model success as timing qualification.

CI builds both demos as US/JP/EU release, US debug and US tour on native ARM64
and AMD64 (ten cases per architecture). Comparison requires all cases, identical
disc/bootstrap payloads and executed raster/HBlank/probe evidence. CI installs
all skills with telemetry disabled, then creates each demo from the installed
standalone package.

## Emulator checks

Load the CUE explicitly as Mega CD with your own region-appropriate BIOS.

Water:

1. Move left/right and hold Up/Down through both waterline limits repeatedly.
2. C enables a continuous sweep; leave it running for several cycles. The line,
   foam, reflected geometry and wave offsets must remain aligned.
3. A toggles actor tint; B toggles waves. Reflection remains visible in both.
4. Start pauses and resumes without a catch-up jump. Cross the foam rows and
   inspect the deliberately clipped actor cells.
5. Watch for whole-screen blue frames, missing reflection, flicker, CRAM dots
   and increasing L/T/S counters. Restore the dry colors every safe VBlank.

Split:

1. D-pad pans the selected view. A selects the other camera.
2. C pans the unselected camera automatically; B compares split on/off.
3. Check each actor's view-relative position, the fixed divider and HUD.
4. Pause/resume, pan to camera limits and repeat the stress in PAL.

FPS counts accepted snapshots per VBlank-clock interval (60 NTSC / 50 PAL),
not host VPS or H interrupt count. H counts consumed boundary requests; L counts
late handler windows, T edge timeouts, S rejected setup intervals. A fixed
startup S increment can occur before the first settled frame; an increasing
counter during normal play is a fault to investigate. FPS alone does not certify
successful raster switches: inspect the effect and the rejection counters.
`TOUR=1` first runs bounded model checks, then continuously sweeps water or pans
the second split camera. It does not synthesize every manual interaction.

## Physical hardware remains required

Qualify CRAM artifacts, precise FIFO/HBlank timing, short level-7 latency,
VBlank completion and worst-case bus contention on real PAL/NTSC machines and
relevant BIOS/model variants. Test with the actual audio driver, CD streaming,
Sub work and any ASIC job active together. Capture multiple boundary crossings
and long sweeps; one clean frame is insufficient. Emulator checks neither
qualify all hardware nor establish spare CPU time. Keep a simpler no-tint or
single-view option where the project cannot afford the effect.

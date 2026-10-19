# Effects validation

`megadev-effects` adds bounded particles, explosions/smoke, trails, weather and
screen shake. Winter Observatory uses original native-resolution procedural
art; its appearance, quiet PSG cues, controls and capacities are illustrative.
It is not a template for a game's visual style or gameplay rules.

## Contracts and measured bounds

The source audit used `ewrogers/megadev:latest`, Megadev revision
`7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef` and GCC 16.2.0. Effects are application
code around actual VDP/BIOS definitions, not SGDK sprite or DMA APIs. They reuse
the existing frame/joypad/Sub interrupt service. No additional drive, mailbox,
ASIC or Word RAM owner is introduced after startup.

- Model: 64 twelve-byte particles; 24 reserved impacts/smoke, 8 trails, 32 weather.
  The complete world is 792 bytes. Rejection is bounded and cannot evict gameplay.
- Timing: 60 simulation ticks/second through a 300 Hz remainder, with at most four
  catch-up steps per call. Pause freezes effects and random state; excess elapsed
  time is deliberately dropped/counts as a clamp.
- Renderer: at most 56 SAT entries, including the 16×16 lantern. 8px effects
  conservatively charge all intersecting eight-line bands. The sixteen-width-unit
  band limit leaves room below H40's 20 entries/320 fetched pixels per line.
- Snapshots: two 806-byte buffers. SAT/scroll/HUD data-port payload is at most
  776 bytes/frame, plus commands/instruction cost. Only the early-VBlank owner
  writes active-frame VDP state. Late callbacks defer the immutable snapshot.
- Resident art: 416-byte hero/effect atlas, 3,072-byte font, 7,136-byte background
  patterns and 2,240-byte source map. Three palette banks are used. Steady-state
  frames do not upload patterns or write CRAM.

These are bounds for the fixed Lab profile, not proof of CPU headroom or
capacities appropriate for every game. The world/HUD separation keeps shake
out of collision coordinates; its envelope restores both axes to zero.

## Executed evidence

Host checks cover PAL/NTSC model equivalence, pause, pool saturation/retirement,
clamped catch-up, deterministic admission, scanline overload, SAT links/culling,
shake recovery, native art reconstruction and safe standalone creation. A
10,000-update combined stress harness runs at O0/O3, with UBSan where supported.

Compiled original-68000 tests execute the model, check exact gravity/movement,
lifetimes and rejection, and exercise the immutable frame publisher. The consumer
must produce every expected SAT/scroll/HUD data word, defer late snapshots,
reject invalid indices/counts and avoid ordinary CRAM writes.

Fresh US/JP/EU release, US debug and US automatic-tour builds pass on ARM64 and
AMD64, with matching target payloads and executed effects/publication evidence.
CI repeats these builds on native runners. Missing execution markers or tour
cases fail report comparison. The disposable install checks all 27 skills with
telemetry disabled. README and skills.sh include effects under Systems.

Emulator observations, regional playtests and CI status are recorded in the PR.
The FPS display counts consumed snapshots, not simulation steps or host VPS.
The automatic tour combines maximum snow and periodic bursts after its model
checks; manual presentation/controls remain a separate audition.

## Limits and lessons

The original 68000 has no native 32-bit multiply/divide instruction. An apparently
small arithmetic expression can emit a freestanding libgcc helper, especially
in debug builds. This Lab uses bounded word arithmetic and explicit MULU/DIVU
where needed; both release and debug linkage are qualified. Avoid extending
that narrow solution into unchecked general-purpose arithmetic.

Ares can reveal missed frames, visual clutter and publication mistakes. Unicorn
proves behavior rather than Sega CD timing. Neither certifies physical VBlank
completion, bus contention or simultaneous audio/CD/ASIC work. Those remain
real-hardware qualification tasks; do not turn a stable standalone FPS reading
or byte bound into a promise of spare CPU capacity.

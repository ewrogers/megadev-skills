# Validation and evidence

## Portable behavior

Run the standalone `tools/check_gameplay.py` with a host C compiler. It executes
at O0 and O3 (with undefined-behavior sanitizer where supported):

- equal-time PAL/NTSC state, bounded catch-up and one-time edge consumption;
- pending input, pause/resume and cooldown clocks;
- wall contact, axis sliding, clear item/spawn boxes and camera clamps;
- bidirectional gates without immediate re-entry;
- full cosmetic pool, rejection, expiry and slot reuse;
- enemy stun, contact damage protection, respawn and one-time completion;
- deterministic long movement runs and unsigned tick wrap.

This proves model behavior, not hardware speed. Art generation is reproducible
at native dimensions; host tests check pattern sizes, legal VDP palette words
and unchanged reused coordination/graphics owners.

## Target builds and execution

Repository CI builds US/JP NTSC, EU PAL, debug and automatic-tour variants in
native ARM64 and AMD64 Docker runners with `--cpus=2`, using `latest`. It compares
complete target payloads and fails if required execution evidence is missing.
Original-68000 and compiler safeguards stay enabled; linker-overflow rejection is
also tested. Local AMD64 validation on an ARM Mac is container emulation, not a
native AMD64 host.

Compiled-68000 execution checks the real movement/model functions and immutable
frame publisher, including late callback deferral, invalid sprite count and no
normal-frame CRAM writes. Paired Main/Sub execution runs movement during healthy,
rejected, cancelled, delayed and ownership-fault requests. The underlying system
regressions check commit order, returned ownership and safe retirement.
MMIO hooks and modeled BIOS calls do not reproduce VDP rendering, CD latency,
interrupt contention, analog sound or console performance.

## ares review

Launch with `--system 'Mega CD'` and the generated CUE. Use the configured BIOS;
never bundle it. Test NTSC and PAL separately:

1. Move around walls and through narrow passages; check facing, walking and
   that the HUD stays fixed while the map and actors scroll together.
2. Collect gems in both areas and use the marked edge gates in both directions.
   Gems should stay collected; clear feedback should trigger once.
3. Pulse near a beetle. It should stop, cause no contact damage while stunned,
   then resume; recharge takes three game seconds.
4. Take contact damage, check brief protection, then respawn after three hits.
5. Start a Sub job while moving. Pause should stop actors and recharge while
   its service can still finish. Reset should not revoke in-flight Word RAM.
6. Check stable HUD, no CRAM dots, finite cues and no stuck sound on pause.

The Lab's automatic tour also starts a healthy job, crosses an area and drains a
deliberately delayed job. It is a repeatable smoke case, not coverage for all
play paths. Mark emulator acceptance only after observation. Real consoles are
still needed for raster margin, PAL/NTSC pitch, controller variants, CD access
and sustained combined-load/stack qualification. There is no measured physical
60 FPS claim merely because the model runs at a nominal 60 Hz.

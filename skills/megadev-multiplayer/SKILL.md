---
name: megadev-multiplayer
description: Build local multiplayer in Megadev Sega CD/Mega CD games with stable player slots, joining during play, shared-camera bounds and independent respawns. Use for party and player-lifecycle policies; device protocols remain with megadev-input and general simulation with megadev-gameplay.
---

# Megadev multiplayer

Start with the user's player count, joining/leaving rules, camera behavior and death/respawn policy. Reuse the game's existing input, clock, renderer and resource owners. Local multiplayer is application logic, not a Megadev multiplayer API. Two 68000s do not imply one CPU per player, and extra RAM does not increase VDP sprite or scanline limits.

## Keep identity separate from input

Read [players and input](references/players.md) for stable socket-to-player claims, first-sample baselining, disconnect grace, deliberate leave and shared menu actions. Poll once per display interval through the existing input owner. Never add a second BIOS or direct reader to obtain another player's buttons.

`megadev-input` owns pad/mouse and Team Player/EA protocols. The standalone Lab includes that project-owned reader for reproducibility; its eight socket positions are a Lab convention, not an SDK player enumeration. A mouse does not become a pad because its buttons share mask bits. Adapter support needs device qualification, not just a fixture or enum.

## Choose camera and recovery policies together

Use [camera and respawns](references/camera-and-respawns.md) to decide who participates in framing, what stops separation, and where players reappear. Keep world state and collision authoritative even when sprites are culled. A shared camera cannot show unlimited separation; choose a tether, regroup, split view or other policy explicitly rather than hiding one player accidentally.

Keep one player's death, cooldown and reconnect separate from other players' state. Bound spawn searches and provide a fallback. Integrate these actions with pause and scene transitions; do not restart the whole room or discard a live Sub job merely because one player leaves.

## Integrate and prove

[Co-op Lab](assets/co-op-lab/README.md) demonstrates four keepers lighting a garden, late joining, a shared camera, white stun-recharge bars and independent returns. Its setting, art, music, controls, movement speeds and game rules are illustrative. Adapt techniques to the user's design, not the Lab's design to their game.

Read [integration and validation](references/integration.md) before adopting the callback, I/O reader or generated disc. It distinguishes Megadev definitions, BIOS services and application helpers, and gives the ARM64/AMD64 and PAL/NTSC test procedure. Model/68000 tests do not qualify peripheral timing, CD latency, audio quality or physical-console performance.

Input owns sampling; gameplay owns the simulation clock; collision owns contact math; graphics/effects own presentation budgets; scenes/streaming/system/memory own resource and Main/Sub lifetimes; save owns persistent encoding; testing owns replay instrumentation. This skill owns party identity, shared actions, framing participation and per-player recovery. It does not implement network multiplayer or invent a link-cable service.

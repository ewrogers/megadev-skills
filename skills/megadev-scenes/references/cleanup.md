# Ownership and cleanup

## Record lifetimes, not just addresses

Keep a short ownership table for the game. Transition Lab uses:

| Resource | Owner and lifetime |
| --- | --- |
| Main/Sub services and scene code | Resident for the whole session |
| Font, actor patterns, PCM samples | Resident; scene cleanup does not evict them |
| Two area pattern slots | Retained bounded cache, 640 tiles each |
| Two Main maps and area palettes | Retained cache, separate from actor progress |
| Current area's collected-lantern value | Scene-local; reset only on a successful scene commit |
| Preload lease | Pins the old scene generation until the reader/mailbox retires |
| Copied frame payload | VBlank owns it until `published` clears |
| 32 KiB read buffer in 2M Word RAM | Reader owns it until the matched result and confirmed return |

The two retained area slots are a small teaching cache, not a claim that every
level should remain resident. For eviction, pin visible/shared slots, account
for queued uploads and ensure rollback retains either the old view or a known
resident recovery screen. Check pattern/table bounds before admitting a scene.

The Lab reuses a fixed scene-state block; it does not free a heap allocation.
Its generation-checked hold/release illustrates preventing late local callbacks
from touching the next scene. It deliberately refuses generation exhaustion
instead of allowing an old handle to become valid again. A production engine
can choose another bounded handle policy with equivalent stale-reference safety.

## Retire producers and consumers

1. Stop admitting new scene-local jobs and gameplay updates.
2. Request cancellation of unnecessary old work. Continue servicing it.
3. Wait for the reader's result, ownership return and mailbox retirement.
4. Wait for any presentation payload still referring to old data. The Lab copies
   upload bytes into Main RAM before queuing them; it does not queue Word RAM pointers.
5. Prepare the target in a non-visible slot. Validate before publishing.
6. After the new view is consumed, reset old local state and advance its generation.

Do not clear mailboxes or change the Word RAM mode to force a transition through.
Do not replace resident Sub code on every screen change. BIOS/CD service and the
access-loop continuation remain active through menus, fades and pause.

## Audio follows its existing owner

The Lab reuses the audio skill's Sub-owned PCM and BIOS CD service. Main sends
desired-track requests; it does not directly compete for the pickup. The reader
acquires the drive only after CD music has stopped and that command completes.
Resident PCM can continue. After the read retires, music can restart from its
beginning; the next scene then selects its desired track. A seamless seekless
CD-music crossfade is not promised.

An audio request acknowledgment means accepted intent, not audible completion.
Likewise, a queued black palette is not a displayed black palette, and a queued
tilemap is not a consumed tilemap. Keep those separate milestones explicit.

Megadev's audited definitions are in
[Sub CD BIOS services](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bios.def.h),
[Sub hardware definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/gate_arr.def.h)
and [Main VDP definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/vdp.def.h).
`scene_*`/`stream_*`/`audio_*` remain project helpers around these contracts.

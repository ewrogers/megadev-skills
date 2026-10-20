# Players and input

Keep three identifiers distinct: physical port/adapter socket, stable logical player number, and actor/pawn handle. Preserve a player's number when a different player leaves. Do not compact an array and silently transfer P2's color, inventory or save identity to another controller. Count players and validate socket indices before indexing fixed storage.

Sample the hardware once through the game's chosen owner, then derive held/pressed/released state. Baseline the first sample and a changed/reconnected device so a held Start does not join, pause or activate a menu accidentally. Treat malformed, absent and timed-out samples as neutral immediately. Never keep a missing pad moving on its last held direction.

The Lab uses four logical players and eight socket positions, 1A–1D / 2A–2D. Ordinary pads occupy 1A and 2A. Team Player readers can populate four positions on each port. EA mode explicitly owns both ports and populates the first four. These are copied project conventions from the Input Lab, not BIOS device IDs or Megadev player constants. Mouse snapshots remain supported by that input reader but cannot join this pad-driven Lab.

Process existing claims before new joins. A new Start joins only; do not also pause or attack in the same frame. When several existing players press a shared pause action together, combine requests once rather than toggling repeatedly. Establish whether pause is global, owned by one player, a vote, or disabled for the chosen game. Menus may need a separate navigation owner.

The Lab reserves a disconnected player's slot for five seconds of bounded service time, freezes that pawn and excludes it from framing. Reconnection of the same reported kind keeps the claim, HP and cooldown; a kind change expires it. Reconnection cannot prove it is the same physical person or controller: pads have no unique serial identity here. After grace expires, a new Start can claim the vacancy. Hold Start+B for one second to leave deliberately. These policies are examples, not required game behavior.

The foreground loop services connection/leave rules while globally paused; simulation timers and cooldowns remain frozen. Extremely long stalls are capped, so its grace is service time, not a wall-clock guarantee. If the project needs real-time disconnect deadlines, use its established service clock instead.

On an area transition retain roster identity and persistent player state separately from room-owned actor handles. Drain jobs and retire resources under the scene/system owners before replacing pawns. Do not reset the input baseline every time a room loads. For replay, record logical normalized per-player inputs and lifecycle events with stable ordering, not only whichever pad became P1 first.

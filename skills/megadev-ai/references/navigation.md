# Bounded navigation and CPU responsibilities

Use direct steering for an unobstructed route, a graph for authored connections,
BFS for a small uniform-cost grid, A* for larger directed searches, or a shared
flow field when many enemies use one target. Choose from the game's geometry
and measurements; do not copy the Lab's 20×12 grid as a world-size limit.

## Concrete Lab bounds

One search owns a 240-byte parent array and a 240-byte queue, plus ten 16-bit
fields: 500 bytes. Every cell is discovered once. Each simulation step expands
at most 12 nodes and tests at most four neighbors per node. A maximum of four
catch-up steps bounds one foreground call to 48 expansions; normal PAL frames
can run two simulation steps. Count this worst case when reserving frame time.
This is a work bound, not a microsecond guarantee.

There are three enemies, each with a 64-byte route, and six projectiles. The
job scheduler rotates its starting enemy, uses one workspace, and backs off
requests for 24 simulation steps. A busy workspace rejects replacement. Empty
frontiers finish with NO PATH. A map revision change cancels a live search.
An output buffer too small for the full route returns failure without partially
overwriting it. The Lab then waits/retries; a game may instead keep a useful
partial path, choose a nearer waypoint, or switch behavior explicitly.

Neighbors have a stable order. Host tests compare route lengths with an
independent shortest-path oracle; deterministic behavior is not evidence that
the first route found is optimal unless the search's cost model supports that.

## Consuming results

An actor's generation changes at state/lifetime boundaries. Installation checks
the owner, generation and map revision. A stun can therefore retire an old
request without it restoring chase movement later. Keep generation wrap and
maximum outstanding lifetime compatible; long-lived asynchronous systems may
need wider counters than this small single-job Lab.

Targets can move during a search. Here the route belongs to a snapshot goal,
while the enemy's latest observed position remains separate. The actor may
finish that route and replan; results never overwrite the newer observation.
If your rules require the latest target only, reject mismatched goal revisions
and rate-limit replans so continuous movement cannot starve every search.

Movement remains responsible for collision and body clearance. On a rejected
step, try a permitted alternate axis, recenter or replan; never teleport through
a solid cell merely because the graph says its center is reachable.

## Optional Sub CPU work

The runnable Lab keeps AI on Main, with resident map/art and no gameplay CD
loads. Do not treat the Sub as a free worker. It also services BIOS/CD/PCM and
shares access to PRG/Word RAM with other producers.

If profiling justifies offload, use `megadev-system` to send a bounded immutable
snapshot: map revision/region, start, goal, actor ID and generation. Reserve a
private workspace and cap service work between BIOS/audio deadlines. Return
value data with a committed completion token. Main validates it before use;
timeout/cancellation does not prove a worker stopped or permit buffer reuse.
Account for handoff/copy cost and target aging when comparing with Main-only
work. No Sub pathfinding adapter is claimed or exercised by this Lab.

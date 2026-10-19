---
name: megadev-ai
description: Implement enemy states, detection, pursuit and bounded pathfinding in Megadev Sega CD games. Use for decision and navigation budgets, including safe optional Sub CPU jobs, rather than general movement, attack resolution or hardware threading.
---

# Megadev AI

Start from the game's enemy rules, geometry, actor count and response targets.
The Lab's courtyard, enemies, stun, goal, colors and sound are examples, not a
recommended genre, aesthetic or set of controls for another game.

Megadev does not supply the Lab's AI functions. `ai_*`, states, route format and
budgets below are application code. Inspect `ewrogers/megadev:latest` before
using SDK/BIOS interfaces; do not import SGDK helpers or assume an AI service.

- Read [States and sensing](references/behavior.md) for transitions, facing,
  occlusion, memory, staggered decisions and readable feedback.
- Read [Bounded navigation](references/navigation.md) for search choice, work
  caps, route validity, fairness, RAM and optional Main/Sub separation.
- Read [Integration and validation](references/testing.md) for actual Megadev
  boundaries, executable tests and evidence requiring a console.

Keep these contracts explicit:

- Simulation and sensing use the game's clock. Detection publishes observations;
  decisions change states; movement consumes intent through the existing
  collision owner. Rendering and sound consume events and never decide hits.
- Specify which directions and distances can detect an actor, whether walls
  block sight, and what happens at touching corners. Reuse collision geometry;
  an unobstructed center ray does not prove a whole actor can fit through a gap.
- Bound the total work, not just each enemy's work. Share a search workspace,
  queue requests fairly, stagger expensive sensing and cap catch-up steps.
  Preserve movement/input while a route is pending. Never busy-wait for AI.
- Tag asynchronous results with actor lifetime and map revisions. A result for
  an old actor or geometry cannot install a route into the current one. Decide
  separately whether a route toward an older target position remains useful.
- Keep actor, path and projectile pools finite. Unreachable goals, full queues,
  short route buffers and canceled jobs need visible, bounded outcomes rather
  than infinite retry loops or silent memory growth.
- Start with measured Main CPU work. The Sub CPU already services CD/PCM/BIOS
  responsibilities; moving AI there is an optional, separately tested design.
  Never share live actor pointers or steal Word RAM to accelerate a search.

[Enemy Lab](assets/enemy-lab/README.md) supplies a native 68000 example, an
incremental grid search, an independent host oracle and a playable courtyard.
Its small uniform grid uses BFS; that is not a universal pathfinding choice.

Use `megadev-gameplay` for timing/movement/pools, `megadev-levels` and
`megadev-collision` for geometry, `megadev-animation` for pose/event timing,
`megadev-system` for inter-CPU ownership, and `megadev-performance`/`megadev-testing`
for measurement and replay. AI adds decisions and route scheduling without
installing competing input, rendering, audio or drive owners.

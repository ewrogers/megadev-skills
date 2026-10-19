---
name: megadev-collision
description: Implement bounded swept AABB collision, finite ray tests and wall sliding in Megadev Sega CD/Mega CD games. Use for fast projectiles, line-of-sight queries, contact semantics, simultaneous hits and nonpenetrating movement; reuse existing gameplay, animation and rendering owners.
---

# Megadev collision

Start with the game's collision shapes, simulation clock and coordinate convention. Megadev provides hardware access, not a general collision engine. The Lab's `cc_*` queries are application code; do not invent SDK collision calls or import SGDK's runtime.

## Choose a query that matches the question

Read [query contracts](references/query-contracts.md) before handling contact. Discrete overlap answers whether two boxes overlap now. A sweep tests motion through an interval, including relative motion of two bodies. A finite ray tests a segment, not an infinite line. Find the first exact contact before applying damage or deciding whether a target is visible.

Keep strict initial overlap separate from a new hit. Treat touching, moving away, parallel motion, endpoint contact and corner grazing consistently. A corner result can block two axes; those flags are not a unit surface normal.

## Bound the work and the arithmetic

Use [bounds and sliding](references/bounds-and-sliding.md) for local coordinates, validated ranges, exact rational ordering, broad-phase rejection and a fixed contact budget. Check every supplied candidate. Reject invalid requests explicitly; do not silently drop walls or classify them as misses.

For static solid walls, move to the first contact, remove blocked displacement components and continue with the remainder. Return unresolved motion if the iteration budget runs out. Initial overlap needs an explicit spawn/depenetration policy. Swept AABB alone does not solve slopes, rotation, one-way surfaces, pushing, platforms or physical bounce.

## Integrate through existing owners

Read [Megadev integration](references/megadev-integration.md). Collision normally runs on Main during simulation, before rendering. Reuse `megadev-gameplay`'s clock and camera; `megadev-animation` owns active hitbox timing; gameplay owns damage, invulnerability and contact response. `megadev-graphics` owns sprite ordering and VDP admission. Loading and Main/Sub coordination belong to their existing services.

Never let camera shake or skipped visual frames change collision. Do not wait for a Sub job every movement step or read Word RAM while the other CPU owns it. Use local queries or an explicitly synchronized snapshot with a bounded result protocol.

## Prove the behavior

[Collision Lab](assets/collision-lab/README.md) contrasts discrete and swept shots, clips rays at the nearest wall, and slides a small rover through obstacles. Its art, sound, controls, shape sizes and speeds illustrate the algorithms; adapt techniques to the user's game rather than copying its design.

Follow [validation](references/validation.md): exact-fraction host oracles, compiled original-68000 execution, PAL/NTSC simulation equivalence and ares presentation checks. Test optimization changes on the actual target instructions. Qualify emulator and physical-hardware evidence separately.

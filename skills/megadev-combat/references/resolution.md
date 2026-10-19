# Attack and damage contracts

## One simulation owner

Read the current attack pose from the animation owner. An attack definition can
supply local boxes, damage, guard class and event IDs; combat transforms the boxes
with actor facing and checks contacts. These are application conventions, not SDK
resource formats. The Lab uses a small integer timeline (6 ticks windup, 7 active,
11 recovery at 60 simulation steps/sec) to keep the example standalone. Replace
it with the game's existing clip timeline rather than running two attack clocks.

Separate feet collision, hurtboxes and weapon reach. The Lab's 10×8 movement box,
12×16 hurtbox and forward 32-pixel active reach demonstrate different purposes.
Do not derive a universal damage box from sprite dimensions. Match visible blade
reach, active poses and collision debug views in the game being made.

## Attack identity and lifetime

A per-swing victim mask works for five fixed actor slots: clear on starting a new
swing and consume a defender bit on its first overlapping contact, including a
block or invulnerable contact. This prevents the tail of one swing hitting when
invulnerability expires. Other games may intentionally retry or have multi-hit
attacks; model separate pulses instead of accidentally counting every frame.

The Lab's serial is diagnostic and may wrap; correctness comes from clearing the
mask when a new instance starts. It never reuses defender slots during an active
round. If a game respawns actors during an attack, store defender generations
alongside hit records or clear affected records as part of slot retirement.
Projectiles carry an owner slot/generation; invalid or recycled owners retire
those projectiles. Owner death alone does not invalidate already-fired bolts in
this example. Choose and test the desired game rule explicitly.

## Gather, then apply

Read all attack windows/defender states, collect contacts and then apply damage.
The Lab allows two active swings to trade. A defender takes at most one damage
unit per step even if multiple swings/bolts contact it; stable actor then bolt
order selects the knockback source. Blocks consume their contact and bolt blocks
retire the projectile. Team checks exclude self/friendly damage before geometry.

Invulnerability suppresses damage, not movement. Hurt cancels active attacks,
disables guarding and produces finite knockback/hit stun. Damage at zero HP stays
zero; dead actors cannot start or continue attacks. Never leave a stale animation
event queued after an interrupt. The example's swing age/state test makes an
interrupted active window immediately inactive.

## Facing and defense

The example guard covers a forward 90-degree cone with inclusive diagonal edges.
Rear and coincident-origin contacts do not block. An incoming bolt is classified
from its velocity; changing the shooter's position after firing cannot turn a
front hit into a rear hit. Stationary/contact hazards need their own guard policy.

The sample opponents expose delayed periodic guard windows and recovery gaps,
rather than perfectly reacting to each bolt. AI chooses when to guard or attack;
combat only resolves the resulting state. Guard stamina, parries, armor, resistances,
combos and damage formulas are optional game rules, not requirements of Megadev.

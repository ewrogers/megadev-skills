# States, sensing and readable decisions

Represent the smallest states that change behavior. A patrol might observe,
alert, pursue, investigate its last observation, return, or be stunned. Another
game may need none of these exact states. Define entry/exit actions and priority:
stun/death usually invalidate pending movement and route results; an animation
event can permit an attack, but the combat owner resolves its damage.

An observation should state who was seen, where and when. A pursuit may remember
the last seen position for a bounded interval. Do not update that position from
the hidden player's live coordinates unless the brief intentionally gives the
enemy that knowledge. Searching should time out or reach a documented fallback.
Avoid switching states every tick near a range boundary: use persistence,
different enter/exit thresholds or explicit transitions where the game needs it.

## Facing and occlusion

Do cheap range/facing checks before geometry. Use bounded integer coordinates;
squared distances and cross products need wider intermediates if their ranges
require them. The Lab uses a Manhattan range and a 90-degree cardinal cone,
then a finite pixel-to-grid DDA. This diamond-shaped range is an example policy.

The DDA visits at most width + height cells. If the line crosses exactly through
a tile corner, touching side walls block it. Test the opposite direction too.
The same solid grid drives movement, so visible art cannot independently promise
an open passage. For large/fast actors, use clearance maps or swept collision;
do not replace those with a center ray.

The Lab samples one of three enemies per 60 Hz simulation step, in round-robin
order. That gives each a 20 Hz sensor with up to two steps of latency. State
timers and movement continue each simulation step. Larger games can prioritize
nearby threats, but distant enemies still need a defined update policy and
occasional service rather than starvation.

## Feedback and attack intent

Show an alert before a threat acts, face it toward its target, and make stun,
recovery and loss of sight understandable. The Lab uses bobbing `!`/`?`, `zZ`,
body flashing for the final stun second, and finite alert/fire/stun cues. None
of these visual or audio choices are requirements for a game using the skill.

Sentry attacks use a finite projectile pool and a cooldown; patrol/hunter body
contact uses a separate hurt grace period. Stunned enemies are harmless. These
small rules exercise decisions and lifetimes, not a general combat system.
An input edge requests a stun; holding A does not repeatedly renew it.

Pause freezes the shared simulation, including sensors, search work, cooldowns,
projectiles and recovery. Hardware services must continue as required. No AI
work runs from an interrupt or from the renderer.

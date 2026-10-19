# Query contracts

`assets/collision-lab/src/collision.h` is the application interface. Boxes use a top-left origin and positive width/height; the Lab uses 16 raw units per pixel (Q4). This is a local format, not a Megadev fixed-point API.

| Query | Meaning |
| --- | --- |
| `cc_overlap(a,b)` | Strict positive overlap now; touching edges do not overlap. Invalid boxes return false. Use the checked sweep/cast APIs when invalid input must be distinguished. |
| `cc_sweep(a,da,b,db)` | Contact over one interval using relative displacement `da-db`. |
| `cc_ray(start,delta,target)` | Finite segment `start` through `start+delta`; zero length is allowed. |
| `cc_cast(a,delta,walls,count,stats)` | First solid contact in a bounded candidate list. |
| `cc_trace(start,delta,walls,count,stats)` | First segment contact in that list. |
| `cc_slide(a,delta,walls,count)` | Bounded movement and response against static walls. |

These are not SDK functions. A sprite's transparent border and visual size need not match its collision box.

## Results

- `CC_MISS`: no entry into the open interior during this interval.
- `CC_HIT`: new contact, with exact `num/den` in [0,1]. `time` is a rounded-down Q12 display value; never use it to order hits or move to contact.
- `CC_OVERLAP`: already strictly inside at the start. No entry normal is fabricated.
- `CC_INVALID`: range, shape or candidate-list contract failed.
- `CC_EXHAUSTED`: sliding used its query budget and still has unresolved displacement.

Touching while entering gives a hit at zero. Touching while moving away or along an edge misses. Reaching a face exactly at the end counts. Touching only a corner for an instant without entering misses. A point inside a box returns initial overlap even when stationary.

`nx`/`ny` are -1, 0 or +1 **blocked-axis flags**. Both may be set at a corner. They are useful for axis cancellation, not normalized reflection. Equal-time walls combine blocked axes; the lowest candidate index supplies metadata. Initial overlaps return the first overlapping candidate, so keep list order stable if it has gameplay meaning.

For line of sight, compare exact blocker and target times. Do not see through the first wall merely because a later target is also hit. Sweeping two moving bodies detects contact in relative space; each body's world position at contact uses its own displacement and the returned fraction. Response between moving solids is a separate problem.

# Snapshot and player patterns

## Keep raw input separate from game actions

`InSample` contains kind, status, held buttons and screen-oriented mouse dx/dy. `IN_OK` means this sample passed its decoder; it is not permanent proof that a device remains attached. A valid idle device has zero held buttons. A timeout is not the same thing as an idle sample.

`InSlot` derives held, pressed, released and menu-repeat masks. Pressed is `current & ~previous`; released is `previous & ~current`. Repeated reads by game systems do not consume or regenerate those edges. Poll once, update once, then pass the snapshot to consumers. Do not let UI, gameplay and audio each run the physical protocol.

After setup, a type change or a failed sample, the first good sample establishes a baseline: held state is visible but it creates no press event or mouse jump. A user who connects while holding Start must release and press again to join. Failure clears held input, reports its releases and invalidates the baseline; the next valid sample cannot fabricate a new action. Tests cover these transitions.

This is an explicit recovery policy, not the only possible game design. A game could retain an authenticated held state briefly, but it must bound that behavior and avoid stuck controls. Do not infer lossless motion or perfectly reliable hotplug from a device ID.

## Menu repeat and opposing directions

The example uses a 300-unit-per-second accumulator: five units per 60 Hz frame, six per 50 Hz frame. Directions act immediately on a change, repeat after roughly 300 ms, then every 100 ms. Host tests compare PAL and NTSC elapsed-second results. Actual repeat times are quantized to frames and depend on the service meeting its cadence.

Opposite Up/Down or Left/Right cancel only for the example's menu-repeat direction. The raw held mask remains intact for inspection. A game's movement, fighting-game SOCD rules, diagonal behavior or last-pressed policy is a separate choice. Do not silently adopt the inspector's rule as the game's controls.

## Mouse motion

Relative motion belongs to one accepted packet. Do not scale it by the display rate, poll it again for another consumer or mistake it for an SDK accumulator. The model drops first-sample motion, applies later deltas once and clamps the cursor to the Lab's small bottom field. A game supplies its own coordinate range, sensitivity, drag, click/double-click and left-handed preferences.

Use actual axis counts for aiming or pointing. Converting them into D-pad bits loses small movements. Invalid/overflow packets have zero motion; consumers should inspect status when choosing recovery behavior.

## Stable socket identity

The eight physical endpoints are `port * 4 + subslot`:

| Configuration | Endpoints |
| --- | --- |
| Direct port 1 device | 1A / index 0 |
| Direct port 2 device | 2A / index 4 |
| Team Player on port 1 | 1A–1D / indices 0–3 |
| Team Player on port 2 | 2A–2D / indices 4–7 |
| EA adapter | 1A–1D / indices 0–3; both ports reserved |

Never compress empty sockets out of the array: unplugging one must not rename the others. In adapter passthrough mode, only the exposed direct endpoint is identifiable; do not invent the physical switch's selected subslot from an ordinary pad packet.

Logical player numbers are separate. The Lab joins on a fresh Start press and assigns the lowest free player number. Disconnects retain the assignment at that socket, allowing it to wait for recovery without another device stealing it. The inspector resets assignments with the program; a real lobby should choose its own explicit leave, reconnect and replacement policy. Joining, multiplayer cameras and respawns are not responsibilities of a physical reader.

## Scripted demonstrations

C toggles labeled scripted Team Player packets. These go through the same decoders/state model but do not assert that a real adapter exists. Physical polling remains separate so C can leave the demo. The script cannot trigger the inspector's own A/B/C controls or change the user's device bindings. It exercises mixed pad/mouse types, edges, repeat and six logical joins.

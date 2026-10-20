---
name: megadev-input
description: Integrate 3/6-button pads, Mega Mouse and Sega/EA multitaps into Megadev Sega CD/Mega CD projects, with verified SDK boundaries, bounded protocol readers and stable input snapshots. Use for device polling, input edges/repeat, socket identity and controller testing rather than game-specific controls.
---

# Megadev input

Inspect the game's existing input owner and selected toolchain before adding a reader. Megadev's direct helper is a **3-button** reader. Device IDs in BIOS headers do not establish a complete six-button, mouse or multitap API. Do not copy SGDK `JOY_*` calls or assume its slot conventions exist here.

## Choose one polling owner

Read [Megadev integration](references/integration.md) before touching ports or replacing a frame handler. The ordinary Main BIOS VBlank path also reads controllers. A second protocol reader can disturb six-button phase state or consume mouse motion twice. Use either the game's established service or one deliberately integrated replacement; consumers read its cached snapshot.

The included Input Lab supplies **project-owned** pad/mouse/Team Player/EA readers using Megadev register definitions. These are examples, not newly discovered SDK APIs. Its custom interrupt preserves the documented Sub-CPU pulse and avoids BIOS input polling. Do not transplant it over a game's existing graphics, audio, raster or communication owner.

## Match the physical device

Use [devices and protocols](references/devices.md) for 3/6-button phases, active-low pad versus active-high mouse buttons, motion signs, handshake limits and adapter port ownership. Team Player and EA 4 Way Play are different protocols. Team Player supports mixed pads/mice on one port; EA owns both ports and is an explicit setup mode here.

Configure only the expected pins as outputs. Restore the idle lines on success and failure. Bound both individual waits and total packet work. Check signatures, lengths and device types before exposing a packet; a missing acknowledgment must not hang the game or retain stale held buttons. Never infer successful hardware support solely from a constant, a cached type or a scripted fixture.

## Publish useful state

Use [input patterns](references/input-patterns.md) for held/pressed/released state, first-sample baselining, menu repeat, stable socket-to-player assignments and mouse cursors. Keep the user's controls and player policy. The Lab's A/B/C/Start inspector actions are illustrative, not a default game control scheme.

Poll once per display frame and share the result. Mouse motion is a per-packet delta, not an accumulated coordinate or a D-pad direction. Consume it once, normalize its axes and clamp the application's cursor. Keep raw button observation separate from gameplay policies such as cancelling opposite directions.

## Prove and qualify

[Input Lab](assets/input-lab/README.md) shows live sockets, normalized masks, input edges/repeat, joined players and mouse cursors. Its labeled scripted mode exercises mixed Team Player packets without pretending adapters are connected.

Follow [testing and evidence](references/testing.md): run host behavior/protocol tests, execute compiled original-68000 code, compare ARM64/AMD64 builds and test available devices in ares. The audited ares version has pads and mouse but no multitap device option. Team Player and EA paths need physical qualification; scripted traffic proves software behavior, not electrical timing or real compatibility.

Project/boot setup belongs to `megadev-project`, hardware facts to `megadev-hardware`, rendering to `megadev-graphics`, and gameplay policy to the game and optional `megadev-multiplayer` for stable party slots, shared cameras and independent recovery. Main owns the base-console I/O ports; the Sub CPU receives snapshots through the game's communication contract rather than dereferencing Main RAM or those port addresses directly.

## Optional remote test input

For a user-requested development-ROM USB controller, the testing skill owns the [optional input hook](https://github.com/ewrogers/megadev-skills/blob/main/skills/megadev-testing/references/usb-input.md). Feed it this input owner's sampled states and route gameplay through its logical result; do not add a second physical sampler, replace SDK globals or reinterpret mouse bits as pad directions. The two USB slots can independently replace pad/mouse state until a short lease expires. This does not test electrical ports, add multitap support or control the BIOS before application entry. The hook is optional instrumentation, not a requirement for ordinary games.

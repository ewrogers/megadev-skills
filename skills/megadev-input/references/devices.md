# Devices and protocols

## Actual Megadev support

The audited `ewrogers/megadev:latest` source is Megadev `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`. Recheck the selected source on upgrades.

| Path | What the audited source establishes |
| --- | --- |
| `READ_INPUT_JOYPAD` in [io.macros.s](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.macros.s) | Raw active-low SACBRLDU read for a 3-button pad. |
| [io.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.def.h) | Data/control registers and low-byte `PAD_*` masks. |
| Main BIOS input mirrors and services | Ordinary two-port hold/hit interface; device detection constants are declared. |
| Mouse/multitap IDs in [bios.def.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.def.h) | IDs `0x03` and `0x07`, not evidence of a complete movement/slot API. |
| Input Lab `in_*` functions | Project-owned extended reader/model, supplied by this skill. |

Two source caveats matter if using the wrappers directly: [io.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/io.h)'s inline reader combines hard-coded D1 rather than its declared scratch operand; `bios_detect_controller` in [bios.h](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/bios.h) binds D6 while the assembly contract in `bios.def.h` says the result is D7. The Lab uses neither wrapper. Do not present them as verified extended-device APIs.

## Pads

A data bit of zero means pressed. The low byte of the Lab's normalized held mask matches Megadev: Up/Down/Left/Right at bits 0–3, B/C/A/Start at bits 4–7. The Lab adds X/Y/Z/Mode at bits 8–11; these upper masks are its own convention, not SDK symbols.

The direct reader captures eight alternating TH-high/low steps, then leaves TH high. The initial pair gives the normal buttons. Low step six must have nibble zero and low step eight must have nibble F before the seventh read's Mode/X/Y/Z nibble is accepted. Checking both avoids treating a 3-button pad's opposing Up/Down state as a six-button signature. Reads are spaced by full frames so the six-button timeout resets phase state.

Keep TR as an **input** while probing/reading a pad: driving bit 5 high would hide C/Start. The familiar Mode-at-power-on compatibility behavior can make a six-button pad behave as a three-button one; do not require boot-time Mode chords for game features.

Sources: [controller phases/timing](https://plutiedev.com/controllers), ares v148 [Control Pad](https://github.com/ares-emulator/ares/blob/v148/ares/md/controller/control-pad/control-pad.cpp) and [Fighting Pad](https://github.com/ares-emulator/ares/blob/v148/ares/md/controller/fighting-pad/fighting-pad.cpp) implementations. Emulator code corroborates its own behavior; it does not qualify every physical pad.

## Mega Mouse

Mouse uses TH/TR outputs and TL acknowledgment. The packet has a B signature, two ignored compatibility nibbles, then flags, buttons and X/Y high/low nibbles. The Lab checks the signature and bounds each acknowledgment wait. The initial device probe already lowers TH; continuing that transaction avoids sampling relative motion a second time.

Mouse buttons are active-high: left maps to the Lab's A, middle to B, right to C and its optional Start to Start. The two-button Sega Mouse lacks some US Mega Mouse buttons, so keep those optional.

Sign bits are separate from the eight movement bits. A negative nonzero byte is extended by subtracting 256; negative zero remains zero. Hardware Y is up-positive; the snapshot negates it for screen-down-positive motion. Overflow makes that mouse sample invalid with zero motion/buttons; it does not invent a different device. The state model clears held state and rebaselines the next good sample. This trades a release on failure for avoiding stuck input; a game may choose another explicit policy.

Source: [mouse protocol, overflow and variants](https://plutiedev.com/mouse), ares v148 [mouse implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/controller/mega-mouse/mega-mouse.cpp).

## Team Player / Sega Tap

In **MULTI** mode, one physical port serves four subslots. The initial high/low nibbles are 3/F. Clocked header nibbles begin 0/0, followed by four types: 0 pad3, 1 pad6, 2 mouse, F empty. Payloads consume 2, 3, 6 or 0 nibbles respectively, in socket order. Even an unused mouse slot's payload must be consumed to keep later slots aligned.

The Lab accepts the complete packet atomically. Unknown types, malformed headers, truncation or timeouts clear that port's four samples. A mouse overflow within a well-formed packet invalidates only that mouse sample. Every exit restores TH/TR high. Two Team Players can provide eight slots; direct devices occupy only subslot A.

Source: [Team Player protocol](https://plutiedev.com/sega-multitap). Physical timing and compatibility remain unverified here. An adapter's passthrough switch positions are not the same as MULTI mode.

## EA 4 Way Play

This adapter owns **both physical ports**: port 1 reads pad data while port 2 selects a socket. Input Lab enables it only through `EA_MODE=1`. Do not simultaneously poll port 2 as a mouse or another pad.

The Lab configures control bytes 40/7F, checks the documented selector-ID heuristic, then collects each TH phase across all four selectors (0C/1C/2C/3C). TH is shared by every pad, so reading a complete pad independently before the next would disturb six-button phase state. Final state is TH high and selector 7C. Only pads are supported. Some Sega adapters offer an EXTRA mode with this protocol.

Source: [EA adapter protocol](https://plutiedev.com/ea-multitap). The selector heuristic is not a general connection guarantee. The software path has scripted and compiled tests, not ares or real-adapter qualification.

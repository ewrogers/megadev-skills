# Planning validation cases

The planning skill reuses the existing Project Lab for boot/build evidence. These walkthroughs exercise decisions that a memory total alone would miss. They do not claim that a new game, streaming runtime or audio driver has been implemented.

## A room fits, but loading it overlaps the old room

The worksheet starts with the user's requirements, not a genre template. A hypothetical project keeps its Main services resident, uses a 96 KiB room module in 2M Word RAM and needs 192 KiB of Word RAM during loading.

These fit as separate phases. Keeping both live during one loading snapshot exceeds 256 KiB. The test adds that overlap and expects a failure. A useful plan must either leave the old module first, stage some content elsewhere, reduce the load or choose a different buffering arrangement. It cannot simply point at the free space in Program RAM without specifying access and copying.

The checker also rejects Main access while Sub owns all of 2M Word RAM. It does not prove a handshake: the project needs an executable module-load and ownership test, such as Project Lab's foundation.

## Two banks do not form one large concurrent buffer

The sample streaming snapshot has 96 KiB for Main and 112 KiB for Sub. Both fit their 128 KiB banks. The test raises Main's demand to 128 KiB + 1 byte while the total remains below 256 KiB, and expects failure.

A project needing larger decoded blocks must split them, stage them differently or reconsider 2M loading. A single combined free-byte number would hide this constraint. ASIC outputs and sample transfers must use the same ownership plan rather than assuming an extra pool.

## A preload window is not a guaranteed load time

The editable example assumes a 250 ms seek, 96 KiB transferred at 153600 bytes/s, 40 ms decode and eight upload frames. Its serial model yields about 1063 ms at 60 Hz, or 1090 ms at 50 Hz. None is a measured drive result.

Tests verify those calculations, a negative margin warning and rejection of invalid/overflowing arithmetic. The plan still needs representative read/decode/upload measurements and a late-load fallback. CD-DA/data contention is a separate design decision, not something the formula simulates.

## Boundaries and inventory

This skill owns feasibility, resource lifetimes and proof criteria. `megadev-project` owns the executable foundation. Future system skills should implement the chosen policies; hardware/testing/release skills should supply evidence for their own scope. Neither worksheet nor checker chooses a game's art, music, controls or progression.

Before merge, validate the skill frontmatter and local links, run all host tests, install both skills with telemetry disabled, and wait for native architecture CI and payload comparison. The public commands continue to use `ewrogers/megadev:latest`; pinned references record the inspected source, not an instruction to silently upgrade a user's project.

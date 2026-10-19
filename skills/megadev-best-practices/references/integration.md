# Owners and integration seams

Harbor Relay is an application example, not a required architecture. Its new code
is the checkpoint adapter and glue; it keeps the existing owners' portable policy
and Main/Sub implementations unchanged.

| Concern | Owner and boundary |
| --- | --- |
| Main elapsed time | One VBlank counter, sampled once by foreground; shared 300 Hz units feed scene/audio/load/save age checks. |
| VDP | Existing copied double-buffered frame publisher; only its callback writes normal-display VDP ports. |
| CD/Word RAM | Existing streaming adapter, immutable copied upload chunks, exact token/ownership retirement. |
| CD music and PCM | Existing audio policy/Sub adapter; data loading acquires its drive lock. |
| Scene lifetime | Existing `Scene` generations and leases; cleanup waits for retirement and consumed publication. |
| Save record | Existing versioned two-record codec and Sub BIOS ABI wrapper; new transport maps only the application fields. |
| Release | Unchanged `megadev-release` helper bundled in `tools/`; validates the mixed-mode delivered artifact. |

The detailed implementations belong to `megadev-system`, `megadev-memory`,
`megadev-streaming`, `megadev-audio`, `megadev-scenes`, `megadev-save`,
`megadev-graphics`, `megadev-performance` and `megadev-release`. Those skills are
useful when available, but this installed example includes its needed resources.
Do not copy SGDK service APIs or cartridge assumptions into these boundaries.

## The mailbox is a resource

The running reader uses command-0 tokens >= 2. Startup's command value 1 has
already retired before application services begin. Checkpoints reserve value 1
only after startup, using the **same mailbox**, with Main-owned command words:

| Command word | Checkpoint use |
| --- | --- |
| 0 | Commit token 1, written last. |
| 1 | Load/save operation. |
| 2 / 3 / 7 | Copied scene / horizontal position / option-progress flags. |
| 4 / 5 / 6 | Left untouched: owned by the audio command channel. |

Sub's status-0 commit token and status 1/2/5/6 carry the checkpoint reply only while
it owns that mailbox. Status 3 remains the Sub tick; status 4/7 remain audio state
and acknowledgement. Register direction and width come from the SDK headers;
these operation numbers and field meanings are application protocol, not SDK APIs.

Main refuses checkpoint work during any reader phase. Its `relay_request` refuses
new reads until the checkpoint's reply handshake has retired, including the small
interval after Sub clears its reply but before Main observes that clear. Raw
mailbox emptiness alone is not the local owner's full state. Sub defers BRAM calls
while a CD access, audio drive operation or drive lock is active. It never borrows
Word RAM for the checkpoint; the BIOS scratch area stays in Sub-owned PRG RAM.

Publish parameters first, then commit; observe the reply commit before payloads;
clear the request, wait for reply clear, then reuse. Holding B is not a queue of
writes: input edges and the pending-operation guard prevent repeated submission.

## Lifetimes and late results

A paused-world save holds the current scene generation. Returning to title may
start a fade, but cleanup cannot finish until both the mailbox and scene lease
retire. The snapshot was copied before submission; later player/option edits do
not change the record being written.

Continue only applies a loaded checkpoint if its originating title generation
and intent still match. Leaving that menu before completion discards the result
without pretending to abort the BIOS operation. Restored position/options apply
after the saved area's map commit, so scene initialization cannot overwrite them.

A 30-second observed timeout quarantines the request and retains its lease. A
late reply does not authorize a retry or free the mailbox. This example asks for
a reset; a game's richer recovery policy must be implemented by the owning
service. Interrupt masking or missed/wrapped clocks still limit timeout evidence.

The integration exposes internal save/load only, with its own `MDBESTLAB_A/B`
filenames. It never formats, deletes or damages a save. Cart support, format
confirmation and deeper codec recovery remain with the save owner. Unavailable or
unformatted backup RAM is reported; it is not silently initialized or redirected.

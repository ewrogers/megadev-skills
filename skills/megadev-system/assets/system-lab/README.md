# System Lab

A standalone Megadev disc demonstrating safe Main/Sub job coordination. It keeps
a Main-side control light and diagnostic display running while Sub transforms a
small owned buffer, rejects bad requests or returns a cancelled/late job.

The `sy_*` protocol is project code. Megadev supplies the register definitions,
BIOS adapters, startup and CD access machinery. The display and controls are
illustrative; they are not a prescribed game framework.

## Create and build

Run the skill's generator into a new directory:

```sh
python3 scripts/create_project.py my_system
cd my_system
docker pull ewrogers/megadev:latest
docker run --rm --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
docker run --rm --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest cue system_lab.iso
ares --system "Mega CD" system_lab.cue
```

Use an absolute source path in Docker's mount. In PowerShell use `${PWD}` for the
current directory; Docker must run Linux containers. Both ARM64 and AMD64 images
are available. Supply your own legal BIOS in ares; none is distributed here.

Use `build REGION=EU VIDEO=PAL` for a PAL build, or `build TOUR=1` for the automatic
tour. Build into a fresh review copy and reload it; never overwrite a disc ares
still has open.

## Controls

| Pad | Action |
| --- | --- |
| D-pad | Move the Main CPU's green control light |
| A | Start a job using the selected test |
| B | Start it with an artificial two-second Sub delay |
| C | Cancel a committed job |
| Start | Cycle Valid Request / Unknown Command / Invalid Size |

A valid job transforms 256 words in chunks of 16, then Main checks every returned
word, header, guard and checksum. Invalid requests return ownership and increment
Rejected. Delayed jobs exceed the one-second deadline: the slot stays reserved,
the reply is discarded, and another job is allowed only after both sides retire.

The automatic tour covers success, rejection, cancellation, timeout/drain and
successful reuse. AUTO PASS expects Accepted 2, Rejected 1, Discarded 2 and Bad
Data 0. It continues updating rather than freezing at the end. Counters are hex.

## Implementation boundaries

- Runtime grant requests are single attempts, with later confirmation checks.
  The initial Project Lab bootstrap still uses synchronous disc-loading helpers.
- Word RAM uses 2M ownership. The payload is 522 bytes, but the handoff reserves
  the whole region. ROOM.MMD is retained as a module-startup test fixture and is
  not loaded by the runtime job service.
- One Main VBlank handler pulses Sub INT2, runs BIOS pad polling, queues button
  edges and commits at most 96 changed text cells plus one hidden SAT entry.
  Deadlines use actual VBlanks, not UI redraws. No palette writes occur in normal
  display. The UI is diagnostic, not a measured 60 FPS game benchmark.
- Sub foreground runs the worker. INT2 keeps Megadev's CD access loop serviced;
  the worker never runs inside that callback. No runtime disc/audio job competes
  for Word RAM in this demonstration.
- A grant that never confirms or a peer that never returns cannot be repaired by
  clearing a local flag. The slot remains unavailable; reset the disc explicitly.

Host tests, compiled 68000 execution and ares provide separate evidence.
Physical handoff/interrupt timing, other BIOS/models, drive and audio coexistence
still need real-console checks. No BIOS or commercial content is included.

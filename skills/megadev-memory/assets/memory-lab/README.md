# Memory Lab

A standalone Megadev disc with three pages: RAM budgets, allocation lifetimes
and safe Word RAM bank exchanges. The visual patterns are diagnostic data;
no external artwork or style is required.

## Create and build

From the installed skill directory:

```sh
python3 scripts/create_project.py my_memory
cd my_memory
docker pull ewrogers/megadev:latest
docker run --rm --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest build
docker run --rm --mount type=bind,source="$PWD",target=/workspace ewrogers/megadev:latest cue memory_lab.iso
python3 tools/report_layout.py --root .
ares --system "Mega CD" memory_lab.cue
```

Use an absolute Docker mount source. PowerShell can use `${PWD}`; Docker must
run Linux containers. ARM64 and AMD64 are supported. Supply your own legal
Sega CD BIOS in ares. Do not overwrite a disc while the emulator has it open.

`build TOUR=1` enables the automatic tour. `build REGION=EU VIDEO=PAL` builds
the PAL variant; compile into a fresh copy for review.

## Controls

Left/Right or Start changes pages.

| Page | A | B | C |
| --- | --- | --- | --- |
| RAM Budgets | — | — | — |
| Lifetimes | Next guided step | Reset if no user holds a lease | Release the simulated deferred user early |
| Word RAM | Prepare data | Enter / leave 1M mode | Swap when the back bank is ready |

On Lifetimes, A loads 3,584 bytes into the 4,096-byte scene arena, refuses an
allocation that will not fit, refuses unloading while pinned, completes the
deferred user, then unloads and rejects the old handle. Releasing early with C
makes unloading legal. This is a lifetime demonstration, not actual DMA work.

On Word RAM, prepare in 2M first, then enter 1M. The CPUs now own different banks.
Main verifies its front while Sub prepares the back. Swap, prepare the new back,
and swap again. Leaving 1M rebuilds valid 2M content before returning ownership.
The display uses a snapshot copied into Main RAM, so no queued VDP upload
references a bank that is being exchanged.

AUTO PASS expects nine completed memory operations, no errors and the lifetime
checks. It continues displaying and counting afterward. Counters/linked sizes
are hexadecimal; physical capacities are labeled KiB.

## Budget report

`layout.json` defines this Lab's reservations. `tools/report_layout.py` checks
actual Megadev `build/*.elf.sym` files for overlap, overflow, wrong origins and
embedded pool bounds. The scene arena is already inside state; it is not counted
again. Broad starter code windows and growth margins are adjustable project
choices. Unassigned RAM is not automatically a heap. Stack usage is reserved,
not measured.

ROOM.MMD is retained only as a startup-test fixture. Its writable state is
outside Main's resident state/stack; runtime bank operations do not execute it.

## Safety and scope

The `ma_*`, `mv_*` and memory command helpers are project examples, not Megadev
SDK allocation APIs. They preserve one mailbox/frame owner and the Sub BIOS/CD
access loop. Runtime handoffs use bounded observations; bootstrap disc loading
remains synchronous.

1M banks are 128 KiB each. Main's 220000 cell-mapped alias is not independent
access to Sub's bank. Mode/bank changes invalidate old views. The Lab rebuilds
content, checks actual ownership and refuses changes while a reader is pinned.
An unconfirmed operation quarantines the view and requires explicit disc reset.

This does not qualify real-console timing, other BIOS/models, direct Word RAM
VDP DMA, active CDC transfers, ASIC work or concurrent audio. No BIOS files or
commercial content are included.

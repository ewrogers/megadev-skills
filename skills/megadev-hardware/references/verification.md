# Choose a safe hardware check

## What each layer proves

| Evidence | Useful for | Does not prove |
| --- | --- | --- |
| Source/header audit | Addresses, flags, API/ABI and module conventions | That the game exercises them correctly |
| Host C tests | Pattern arithmetic, bounded loops, corruption and guard checks | 68000 instructions, bus access or BIOS behavior |
| Executed M68000 code | Compiled startup/probe behavior, real target endian/ABI and register use | Two CPUs, Gate Array ownership, drive/interrupt timing |
| Ares run | Integrated boot, command/handoff behavior and visible outcomes in that emulator | Physical drive behavior, board revisions or electrical details |
| Real console | Observed behavior on that precise setup | Every BIOS, region, drive, disc and accessory |

Record the exact image/source revision and settings that matter. Native ARM64/AMD64 builds establish host-toolchain consistency; they do not represent two Mega CD hardware models.

## Hardware Lab protocol

A creates four tests:

1. Main writes/verifies patterns in its 128-word scratch array.
2. Main writes a seeded pattern into 512 words of Main-owned 2M Word RAM, then grants it to Sub and publishes a command.
3. Sub checks its own 128-word scratch array, verifies Main's pattern, writes a different seeded pattern and returns Word RAM before completing the handshake.
4. Main checks mode/ownership and verifies Sub's returned pattern.

The Word RAM probe covers 1024 bytes, not the full 256 KiB. Each local probe covers 256 bytes. The patterns vary by address and seed; this is not an exhaustive address-line, timing or physical-memory diagnostic. No backup RAM, PCM sample RAM or VDP RAM is swept.

B echoes a value through the command channel. C loads a Main module into Word RAM, calls it and returns. The module dirties its own initialized data/BSS before leaving; repeated reloads should still show fresh startup PASS checks. The loader deliberately replaces the previous scratch contents only after the Main program has stopped using them.

The optional automatic tour runs three A checks, one B ping and two C reloads, then shows AUTO TOUR PASS only if all results remained good. It replaces the controller input source during that build. It is useful for runtime QA but cannot certify input bindings. The normal build remains manual.

## Failure handling

If mode/ownership is wrong, do not read or write the buffer to learn more. Capture status and stop that operation. If command completion times out, keep uncertain resources unavailable. Do not blindly reset Sub, clear all communication registers, retry the same command or free a buffer that may still be in use.

The Lab uses the project's existing boot/BIOS/grant helpers. Some have blocking loops. Its 180-foreground-frame runtime handshake timeout is not a bound on the whole system or a measured real-drive deadline. A production loader needs a recovery design suited to its BIOS and resource ownership.

Unexpected FAIL results may be software defects, a bad memory map, stale modules or emulator issues; do not immediately diagnose a damaged console. Keep loaded discs immutable, rebuild outside the active image, and reload the complete disc after code/import changes.

## Real-hardware qualification still needed

A useful record identifies console/add-on model, BIOS region/version, video mode, loader/medium and exact build. Test cold boot and repeat loads before harder cases. A flash-cartridge disc loader, burned disc and emulator can have different access behavior; record which was used.

For the relevant systems, add physical tests for drive seeks/latency/retries, PAL/NTSC timing, real controllers and multitaps, PCM/CD-DA/FM/PSG coexistence, DMA/bus pressure, ASIC completion/ownership, save-media capacity and failure handling. Preserve existing saves and never auto-format media.

Do not fill a checklist with PASS from unrelated evidence. Mark untested systems and distinguish a sampled scratch region from full-memory coverage. If a test requires hardware you do not have, say what remains to be measured.

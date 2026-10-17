# Budget checker format

The helper is a Python 3.10+ arithmetic aid with no third-party dependencies. It does not require Docker. Copy `assets/budget-example.json`, replace its teaching estimates, and run the installed skill's `scripts/budget_check.py` on that file. Add `--json` for a machine-readable report.

Exit codes: **0** means declared demands fit; **1** means a declared capacity, ownership or timing check failed; **2** means invalid input. Warnings do not turn 0 into a hardware-feasibility claim. Missing CPU/graphics evidence stays visible as a warning.

## Top level

- `version`: integer 1.
- `display`: `mode` H32/H40 and integer `refresh_hz` 50/60.
- `phases`: nonempty list of uniquely named operating snapshots.
- `allocations`: named byte reservations and their active phases.
- `loads`: optional assumption-based loading calculations.
- `notes`: optional explanation.

Unknown fields are rejected at each defined object level, so a misspelled capacity field cannot quietly disappear.

## Phases

Each has `name` and `word_mode` (`2m` or `1m`). A 2m phase also needs `word_owner` (`main` or `sub`). Omit that field for 1m, where each CPU has its own bank.

Optional `graphics` contains all four nonnegative integer demands: `palette_lines`, `sprites`, `peak_line_sprites`, `peak_line_pixels`. Count simultaneous palette lines and hardware sprite pieces/width, not actor counts or nontransparent pixels. These are supplied worst-case demands, not values inferred from art or positions. No raster/overscan/interlace variant is modeled.

Optional `cpu_us` contains `main` and `sub`, each a nonnegative finite number. Supply measured worst-case per-frame costs including relevant waits/interrupts. Each is compared independently with the nominal frame period; the checker cannot tell whether two declared tasks can overlap in reality. `notes` may explain the measurement.

## Allocations

Each has:

| Field | Meaning |
| --- | --- |
| `name` | Unique allocation identity. |
| `pool` | work, program, word, pcm, vram or z80. |
| `bytes` | Nonnegative integer bytes per allocation, including required padding. |
| `count` | Optional positive integer multiplier; default 1. |
| `owner` | main for work; sub for program; main/sub for word; pcm, vdp or z80 for those dedicated pools. |
| `phases` | Nonempty list of unique known phase names. |
| `evidence` | estimate, measured or reservation. |
| `notes` | Optional explanation/source. |

The helper sums active allocations separately in each phase and reports the peak for each physical pool. It does not sum exclusive phases. Include BIOS space, stacks and table reservations explicitly; nothing is silently deducted. No allocation is inferred from a filename or an evidence label.

For Word RAM, `owner` means the CPU needing access in that phase. A 2m phase rejects access by the other CPU. In 1m mode each CPU is capped at 131072 bytes, in addition to the shared physical capacity. Describe an allocation that changes owner with separate names in disjoint phases. If bytes are retained without access during a handoff, account for that storage under the current owner and explain it in notes. Do not double-count the same physical bytes under two names.

The example's play/load/stream phases illustrate alternative arrangements. They do not prove a runtime can switch among them without quiescence, copying or address changes.

## Optional loads

Each load has `name`, integer `packed_bytes`, `assumed_seek_ms`, positive `assumed_bytes_per_second`, `assumed_decode_ms`, integer `upload_frames`, and `available_ms`. Times/rates must be finite nonnegative numbers. Optional `notes` should say where the assumptions came from.

The serial model is seek + packed transfer + decode + upload frames. Upload frames use the selected refresh rate. The report shows modeled time and available-time margin. A negative margin warns to plan a fallback. The model is deliberately simple: it does not simulate pipelining, repeated seeks, retries, audio contention, BIOS overhead or a real drive.

Packed/decoded/scratch storage still belongs in `allocations`; a load entry does not allocate memory or deduce decompression sizes. CPU, palette, sprite and transfer inputs also need independent measurement or asset analysis.

## What PASS does not mean

The helper does not check linker addresses, aliases, alignment, fragmentation, stack depth, memory access mappings, loaded-code imports, VBlank/DMA scheduling, protocol correctness, backup-RAM file capacity or real hardware. Free bytes cannot be moved from one physical pool to another. A complete game plan pairs this report with source inspection and executable proofs.

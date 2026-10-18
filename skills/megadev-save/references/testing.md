# Save validation

## Executable checks

Host C fixtures run at O0/O3 and use undefined-behavior checking where supported.
They cover CRC's standard known vector, every single-bit record corruption,
guard bytes, output preservation, alternate-slot writes, damaged-newest fallback,
v1 migration, valid future versions, equal-sequence conflicts, sequence exhaustion,
unexpected metadata, IO failures, failed writes and readback mismatch.

Original-68000 tests execute both actual Main/Sub adapters and BIOS assembly
shims. A named-file BIOS model checks `_BURAM` function codes, resident work
buffers, carry handling, A0 restoration after search, exact filenames, normal
64-byte metadata and D1.l=0 on writes. Oversized/protected files never reach
BRMREAD. Unavailable/unformatted/other-format media reaches only BRMINIT. Delete
and format calls are rejected by the model. Mailbox checks cover writer direction,
no overwrite while busy, result retirement and lost-reply quarantine at 50/60 Hz.

US/JP/EU release, US debug and a read-only US tour build on ARM64/AMD64. Compare
target payloads and startup/execution facts. The tour only probes media; it never
automatically writes or damages a real emulator save. Distribution installs the
skill with telemetry disabled and generates it without repository helpers.

## Ares persistence and recovery

Open the CUE with `--system 'Mega CD'` and your configured BIOS. Confirm formatted
media, edit values, save, reset working values and load. Save a second distinct
record, then explicitly run the Lab's damage/recover gesture. Check that the
older values return, not the newest values or defaults.

Quit/reopen the emulator normally, then load again to test persistence beyond
the running process. Ares v148 stores internal BRAM through the system package's
`backup.ram`; the MIA front end maps it to a `.bram` file for its selected system location. Inspect the selected
version/configuration instead of hardcoding a filename from another emulator.
Do not overwrite or convert existing save files to run a test. If media is
unformatted, let the user manage it through the BIOS; the Lab refuses writes.

Other emulators can use per-game, per-BIOS, shared or combined files and optional
cartridge configurations. Filename extension alone does not establish compatible
size, byte order or layout. Qualify conversion separately and preserve originals.

## Physical limits

Main cartridge tests use a separate named-file store and the actual `BRAM_CART`
adapters. They cover standard IDs, absent/custom IDs without BIOS dispatch,
unformatted media, independent saves/readback/recovery and device selection
without falling back to internal writes.

`tests/cart_bios.py PROJECT --bios YOUR_BIOS --id 4` is an optional local check
against the precisely audited user-provided firmware. IDs 1/4/6 were executed
for 16/128/512 KiB. It runs the original 68000 BIOS and compiled Main adapters on
a fresh in-memory bus model. Its format call seeds only that new Python bytearray;
it never touches emulator save files or hardware. No BIOS is distributed or
required by CI. This is stronger ABI evidence than a stub, but not a full console
or electrical-timing simulation.

The BIOS model does not prove battery health, power-loss behavior, physical write
timing, filesystem repair or behavior across BIOS/model variants. Ares runtime
checks do not establish those either. Qualify simultaneous CD/audio/ASIC work,
full/fragmented media and cartridge insertion/size/write protection separately.
Internal and standard external-cart BIOS saving are implemented. Interactive cart
validation needs a cart-capable emulator/hardware; ares v148 lacks that mapping.
Copying between devices and custom Work RAM handlers remain unimplemented.

[Ares v148 MIA Mega CD storage mapping](https://github.com/ares-emulator/ares/blob/v148/mia/system/mega-cd.cpp) verifies the package/file mapping; user paths and sharing policy still depend on the front-end configuration.

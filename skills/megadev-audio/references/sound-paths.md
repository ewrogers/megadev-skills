# Sound paths and authoring

Audited Megadev revision: `7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef`.
Inspect the image again when that changes.

| Path | Megadev support inspected | Good fit |
| --- | --- | --- |
| CD-DA | Sub BIOS definitions/wrappers, mixed-mode CUE documentation | Full arrangements, recordings and long tracks |
| Sega CD PCM | `sub/pcm.h`, `pcm.def.h`, `pcm.s` | Resident effects, loops, sampled instruments/voices |
| FM/PSG | Base-console hardware and Z80/register definitions; no bundled sequencer found in the audited source | A separately selected/integrated driver or deliberately scoped register implementation |

## CD audio

Primary sources: [disc mastering](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/docs/disc.md),
[Sub BIOS definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bios.def.h),
[Sub C wrappers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/bios.h).
Actual services include `BIOS_MSC_PLAY` (continues through tracks),
`BIOS_MSC_PLAY1` (once), `BIOS_MSC_PLAYR` (repeat), `BIOS_MSC_STOP`,
`BIOS_MSC_PAUSEON/OFF` and `BIOS_CDB_CHK`. Play takes A0 pointing to a track word;
CDB_CHK's carry clear means the command completed. Completion does not mean the
music finished. The Lab uses C ABI shims around these definitions, preserving
callee-saved registers and a resident track argument. These are not new SDK APIs.

CD-DA uses stereo, 16-bit, 44.1 kHz PCM tracks. Keep WAVs outside DISC_PATH:
track 1 is the ISO data track, tracks 2+ are AUDIO entries in a mixed-mode CUE.
Megadev documents a physical two-second silent pregap in each WAV, with
`INDEX 00 00:00:00` and `INDEX 01 00:02:00`. Merely writing the indexes does not
insert silence. Preserve an existing project's track layout; do not overwrite a
mixed-mode CUE with the toolchain's data-only `cue` command.

Audio Lab's `make_audio.py` creates original illustrative arrangements with
headroom and softened ends; `make_cue.py` checks format, whole 588-sample sectors,
actual pregap and minimum duration before writing its known layout. A different
project may use a different mastering policy; audit it rather than rewriting
commercial/imported disc layouts. The ISO inspector validates the data track,
not the audio; tests separately validate/hash the WAVs and CUE.

## Sega CD PCM

Sources: [register definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.def.h),
[helpers](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.h),
[assembly settings/settle delays](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/sub/pcm.s).
The chip has eight channels sharing 64 KiB sample RAM. Sub accesses 4 KiB wave
banks through `_PCM_RAM` on the odd byte lane. `WAVEBANK(0..15)` and
`CHANNEL(1..8)` construct control values; channel selection is **one-based** in
Megadev's macro. Keep the global enable bit set when selecting banks/channels.
`PCM_CDISABLE` bits set disable channels. Register selection is shared state;
interrupt/foreground code must not race to select different channels.

The RF5C164 representation has bit 7 positive and seven magnitude bits; a clear
sign bit is negative. `0x80` is zero; `0xFF` is the loop marker. The Lab encoder
clamps symmetrically to magnitude 126, so sample payload never contains `0xFF`.
It appends markers deliberately. Start is in 256-byte pages; loop is a 16-bit
sample address. Keep both inside reserved RAM. One-shots jump to a reserved
zero/marker loop, then the manager explicitly disables the voice after its
bounded lifetime; looping is not automatic channel allocation or release.

With the 12.5 MHz / 384 sample clock and 11 fractional address bits,
`step = round(sample_rate * 2048 * 384 / 12,500,000)`.
The 8 kHz bed uses 503; 11,025 Hz effects use 694. This PCM clock is separate
from PAL/NTSC VBlank. The [ares RF5C164 implementation](https://github.com/ares-emulator/ares/blob/v148/ares/md/mcd/pcm.cpp)
corroborates the emulated encoding, loop and step behavior; it is not physical
chip qualification. Compare intended pitch/duration by ear on real hardware.

Reduce clipping before encoding, keep attack/release/loop seams intentional,
and audition at the intended rate. Lower sample rates save RAM but reduce high
frequency detail. More channels still sum into a finite output range; preserve
mix headroom. Keep external artwork/music/sample licensing separate from format
validation. The Lab's generated sounds do not prescribe a game's style.

## FM and PSG

The audited [Z80 definitions](https://github.com/drojaazu/megadev/blob/7a7246c14b845ad2f1bd3c7d73afb04cf67d83ef/lib/main/z80.def.h)
and helpers expose bus/reset and chip addresses. They are not a music driver.
Choose a real compatible driver before teaching its queue/data format; verify
license, base-68000/Z80 build, timer/interrupt ownership and residency. Do not
invent `SND_*`/`XGM_*` calls in Megadev. If writing direct register sound, scope
it explicitly, honor YM busy/write timing and coordinate with the existing Z80
owner. Do not reset or halt a live sound driver merely to play a cue. This Lab
exercises CD-DA and Sega CD PCM, not FM/PSG playback or an imported driver.

# Evidence and failure tests

Test the independent contracts as well as a complete movie. A successful disc
build does not establish that frames appear or that sound is synchronized.

- Offline: validate fixed dimensions, palette legality, all tile payload lengths,
  exact sector-rounded files, header fields, frame indices and sums. Re-export
  twice and compare bytes; changing a source pixel must affect the correct tile.
  Check temporal palette/dither flicker by watching actual playback.
- Portable C: reject oversized/odd/empty uploads, partial-frame flips and invalid
  frame order. Compare PAL/NTSC presentation clocks, pause and delayed-frame
  holds. Execute O0/O3, not just a Python rewrite of the C logic.
- Original-68000: execute table setup, Main grants/receive/check/copy, Sub file
  lookup/async completion/cache/prefetch/Word RAM copies, actual publication and
  the presentation consumer. Run every frame; require each group is read once.
  Check late/active-display packets defer, malformed ranges fail closed and
  timeout leaves a live request outstanding. Capture exact DMA source bytes.
- Integration: US/JP/EU release, US debug and automatic tour on both native CI
  architectures. Compare all movie files and boot/module payloads. Report
  comparison must reject absent execution evidence or missing frame groups.
- Ares: verify no flickering/partial pages, readable HUD, actual IMAGES/SEC, slow
  mode holds, audio, pause, skip during a group read and replay after drain.
  Repeat in PAL and NTSC. JP is a disc-region build, not translated text coverage.

`tools/check_cinema.py` works after installation without other skills. It needs
a host C17 compiler. `tests/test_video.py` adds the independent exporter/header
and timing tests; `tests/video_cpu.py` executes both compiled processors and
models CDC, DMA, Word RAM handoff and PCM registers. These models do not emulate
physical drive seeks, bus contention, pixel output, PCM playback or DMA duration.

Hardware qualification remains explicit: drive seek/fragmentation/retry latency,
VDP FIFO/VBlank/DMA limits, overscan, color and temporal artifacts, PCM quality,
combined workload and skip/reset recovery. Test on the intended models and BIOS
regions. Do not claim 10 image/sec on real consoles from a timer constant or a
fast emulator. A general codec or synchronized long-voice extension needs its
own malformed-stream, worst-case decode/refill and audio-clock regressions.

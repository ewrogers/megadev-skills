# Optional voice without a second audio system

Choose an audio strategy deliberately. Mega CD PCM has 64 KiB of sample RAM and
eight hardware channels. It is separate from Word RAM, Program RAM and VDP VRAM.
CD capacity does not make PCM residency unlimited. Longer dialogue can preload or
stream through the existing reader/audio owners; CD-DA playback shares the drive
with data access and is not a zero-latency random-access voice cache.

## The resident demonstration

The Lab reuses the audio policy, Main mailbox and Sub PCM/BIOS adapters. It extends
that profile with `AU_SPEAK` and `AU_SILENCE`, not a competing service. The existing
command slots 4–6 and status 7 retain their publication/retirement handshake.
Application `Speech` generations track line intent on Main; the audio service's
sequence number remains the transport identifier. They are different contracts.

The first greeting and the first paired-panel line have short synthetic excerpts.
Other lines remain unvoiced. Sub initializes all sample banks before normal use.
The four original cue banks use `0x0000..0x3FFF`. Two reserved speech slots use
`0x4000..0x7FFF` and `0x8000..0xBFFF`, leaving `0xC000..0xFFFF` free. Each excerpt
is at most 12,000 bytes at 8 kHz and has an explicit finite lifetime. Its end marker
loops into the existing encoded-silence guard. No live voice bank is overwritten.

Speech uses hardware channel four; the existing effect voices use channels two
and three. `AU_SILENCE` stops only speech. It is not the global panic command.
The pitch step, start-page alignment, bank selection, sign/magnitude encoding,
reserved `0xFF` marker and register settling follow the selected PCM interface.
No speech synthesis, sample resampling or DSP runs in the game loop.

Committed mono PCM16 WAVs are the authoring sources. `make_audio.py` converts them
using integer interpolation and explicit amplitude/edge limits, rejecting stereo
or oversize sources instead of truncating them. Optional `make_voices.py` uses
eSpeak NG to create new source WAVs in a new folder. Its exact voice depends on
that tool/version; rebuilding the ROM needs neither eSpeak nor an API key.

## Latest intent and safe retirement

1. A presented line requests a clip and generation. A pending command keeps its
   original arguments immutable, even if the conversation has moved on.
2. If an old start retires after cancellation, record that it may have started.
   Request a speech-only stop before starting the newest line.
3. Allow the new start only after the stop handshake has retired. Do not queue
   every skipped line or free a sample merely because it is no longer wanted.
4. On a lost acknowledgement, retain the in-flight request and mark voice
   unavailable. Subtitles and choices can continue. Timeout is not cancellation.

The Main adapter also blocks generation rollover while renderer/audio requests
remain outstanding. A caller replacing that adapter must retain this drain rule;
wrapping a small identifier alone does not retire an older operation.

A skip may take the existing mailbox's retirement latency to silence an already
committed start. Do not promise instantaneous cancellation across CPUs. These
samples remain resident for the whole Lab; a streaming implementation additionally
needs leases until the stop/completion contract proves storage reusable.

The Lab does not demonstrate runtime voice reads, seeking, CD-DA dialogue or a
full mixer. Its paired CPU tests inject BIOS completion states; optical-drive
latency, analog quality and real PCM contention need emulator/console listening.

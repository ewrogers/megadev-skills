# Streamed voice source

`stream_voice.wav` is a mono PCM16, 8 kHz synthesized reading of this original
Lab text, not a recording of a person or a copied game sample:

> Welcome aboard the harbor relay. This voice is longer than the sound chip can
> hold. Small pieces arrive from the disc while the display keeps moving. Sound
> effects still play from their own memory. If a read arrives late, we wait
> quietly, then continue. The next music request returns when the voice finishes.

Authored offline with Debian's eSpeak NG (`en-us`, 155 words/minute), then
band-limited/resampled with FFmpeg/libswresample to 8 kHz PCM16. The repository
contains the resulting sound, not the speech engine or voice database. eSpeak
NG is a separate GPLv3-or-later tool: https://github.com/espeak-ng/espeak-ng.
Rebuilding the Lab requires neither eSpeak nor FFmpeg; it uses this WAV and
Python's standard library. The original text and Lab assets are provided under
this repository's MIT license. Keep licenses for substituted voices/samples.

`make_voice.py` makes the other long sample procedurally from sine/chime tones.
The voice, music and chimes illustrate playback behavior; they do not prescribe
a game's mood, voice, musical style or sample rate.

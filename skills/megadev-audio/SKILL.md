---
name: megadev-audio
description: Implement and troubleshoot Megadev Sega CD audio using verified CD-DA BIOS services, PCM samples and explicit sound/drive ownership, including authoring, channel policy and data-load handoffs.
---

# Megadev audio

Choose the sound path from the game's needs, not the Lab's music or appearance.
Sega CD adds CD-DA and an eight-channel PCM chip; the base console's FM/PSG still
exist. These paths have different data formats, owners and loading constraints.

- Read [Sound paths](references/sound-paths.md) for actual Megadev APIs,
  sample encoding, mixed-mode discs and the FM/PSG boundary.
- Read [Runtime ownership](references/runtime.md) for voice priorities,
  Main/Sub requests and CD music/data handoffs.
- Read [Validation](references/testing.md) for asset checks, executable tests,
  PAL/NTSC auditions and remaining hardware evidence.

Use `ewrogers/megadev:latest` and inspect its selected source before teaching an
API. The audited revision and primary links are in the references. Do not port
SGDK XGM/SND calls or imply that Megadev bundles an equivalent music driver.

Essential contracts:

- One owner selects PCM channels/wave banks and manages channel-enable bits.
  Reserve sample ranges; disable their users before replacing bytes.
- PCM chip samples are sign/magnitude with a reserved loop marker, not WAV or
  signed two's-complement PCM. Bound uploads, loop/start addresses and pitch steps.
- CD audio belongs to real audio tracks after the data track. A WAV inside an ISO
  is a data file, not a CD-DA track. Validate the CUE and its referenced audio.
- One physical drive cannot independently seek for CD music and data reads.
  Stop/complete the music command before granting the reader; restart or change
  music after the read retires. Resident PCM can provide continuity.
- Keep BIOS/CD service alive while Main changes pages, fades or pauses gameplay.
  A mailbox acknowledgement is not audible playback completion. A timeout must
  not authorize another drive command or reuse of a still-owned buffer.

The self-contained [Audio Lab](assets/audio-lab/README.md) has a generator at
`scripts/create_project.py`, original music/PCM authoring tools and a mixed-mode
CUE builder. Its `audio_*`/`stream_*` helpers are project code, not SDK APIs.
Build/test containers use at most two CPUs.

Reuse the game's existing system, memory, streaming and graphics owners.
`megadev-streaming` covers the reader and staged uploads; this skill covers
sound policy and drive arbitration. It does not require a particular renderer,
musical genre, art style, controls or scene architecture.

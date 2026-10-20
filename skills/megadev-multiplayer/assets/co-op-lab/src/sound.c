#include "coop.h"
#include <system.h>
/* Lab-only PSG owner. A game's audio service should consume these events. */
#define PSG (*(volatile unsigned char *)0xc00011u)
static MpU16 initialized, previous, note = 65535, left, win, win_note = 65535,
                                    cue;
static void tone(MpU16 channel, MpU16 period, MpU16 volume) {
  PSG = (unsigned char)(0x80u | (channel << 5) | (period & 15u));
  PSG = (unsigned char)((period >> 4) & 63u);
  PSG = (unsigned char)(0x90u | (channel << 5) | volume);
}
static MpU16 pitch(MpU16 p) {
#if VIDEO == PAL
  p = (MpU16)(p - (p >> 7));
#endif
  return p;
}
void lab_sound(MpU16 tick, MpU16 event, MpU16 paused) {
  if (!initialized) {
    for (MpU16 i = 0; i < 4; i++)
      PSG = (unsigned char)(0x9fu | (i << 5));
    initialized = 1;
  }
  if (paused) {
    PSG = 0x9f;
    PSG = 0xbf;
    PSG = 0xdf;
    note = 65535;
    previous = tick;
    return;
  }
  MpU16 delta = (MpU16)(tick - previous);
  previous = tick;
  if (delta > 4)
    delta = 4;
  if (event & MP_CLEAR) {
    win = 180;
    win_note = 65535;
    left = 0;
  }
  if (win) {
    static MpU16 const melody[15] = {340, 285, 214, 170, 214, 170, 143, 0,
                                     170, 143, 127, 0,   143, 127, 107};
    MpU16 n = (MpU16)(180u - win) / 12u;
    if (n != win_note) {
      if (melody[n]) {
        tone(0, pitch(melody[n]), 5);
        tone(1, pitch(melody[n] * 2u), 9);
        tone(2, pitch(melody[n] * 3u), 12);
      } else {
        PSG = 0x9f;
        PSG = 0xbf;
        PSG = 0xdf;
      }
      win_note = n;
    }
    win = delta >= win ? 0 : win - delta;
    if (!win) {
      PSG = 0x9f;
      PSG = 0xbf;
      PSG = 0xdf;
      note = 65535;
    }
    return;
  }
  MpU16 phase = (tick >> 4) & 31u;
  if (note != phase) {
    static MpU16 const song[32] = {
        428, 0, 340, 285, 0, 340, 214, 0, 255, 0,   285, 340, 0,   428, 340, 0,
        382, 0, 303, 255, 0, 303, 191, 0, 214, 255, 303, 0,   340, 0,   428, 0};
    MpU16 p = song[phase];
    if (p) {
      tone(0, pitch(p), 11);
      tone(1, pitch(p * 2u), 14);
    } else {
      PSG = 0x9f;
      PSG = 0xbf;
    }
    note = phase;
  }
  if (event) {
    left = (event & MP_JOIN) ? 24 : 12;
    cue = event;
  }
  if (left) {
    tone(2,
         pitch((MpU16)((cue & MP_HURT   ? 440
                        : cue & MP_JOIN ? 180
                                        : 120) +
                       (left << 3))),
         7);
    left = delta >= left ? 0 : left - delta;
  } else
    PSG = 0xdf;
}

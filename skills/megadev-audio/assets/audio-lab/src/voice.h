#ifndef LAB_VOICE_H
#define LAB_VOICE_H
#include "audio.h"
/* Application policy, not a Megadev API. Half 2 means the silence loop. */
enum { VS_IDLE,VS_BUFFER,VS_PLAY,VS_DRAIN,VS_DONE,VS_FAULT };
#define VS_HALF 16384u
#define VS_SILENCE 0x3f00u
typedef struct {
    A16 phase,total,next,completed,current,ready[2],block[2],underruns,rejected;
} VoiceStream;
int voice_begin(VoiceStream *s,A16 blocks);
int voice_target(VoiceStream const *s);
int voice_commit(VoiceStream *s,A16 half);
void voice_observe(VoiceStream *s,A16 half);
int voice_resume(VoiceStream *s);
A16 voice_loop(VoiceStream const *s);
void voice_stop(void);
void voice_request(A16 sample,A16 slow);
void voice_sub_tick(A16 now,A16 hz);
int voice_busy(void);
A16 voice_status(void);
/* Only audio_sub.c owns the channel-enable shadow. */
void audio_stream_enable(int enabled);
extern VoiceStream voice_stream;
#endif

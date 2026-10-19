#ifndef AUDIO_LAB_H
#define AUDIO_LAB_H
/* Original application policy; none of these audio_* functions is an SDK API. */
typedef unsigned short A16;
typedef unsigned int A32;
enum { AU_PLAY=1,AU_PAUSE,AU_STOP,AU_EFFECT,AU_BED,AU_PAN,AU_PANIC,AU_SPEAK,AU_SILENCE };
enum { CD_NONE,CD_STOP,CD_PLAY,CD_PAUSE,CD_RESUME };
typedef struct {A16 remaining,priority,sample,pan;} AudioVoice;
typedef struct {
    A16 desired_track,desired_pause,applied_track,applied_pause,data_lock,inflight,pending_track;
    A16 bed,pan,ended_mask,dropped,fault;
    AudioVoice voices[2];
} AudioState;
int audio_command(AudioState *s,A16 op,A16 arg);
int audio_voice(AudioState *s,A16 sample,A16 priority,A16 ticks,A16 pan);
void audio_tick(AudioState *s,A16 elapsed);
A16 audio_drive_next(AudioState *s);
void audio_drive_complete(AudioState *s);
extern AudioState audio_state;
void audio_init(void);
void audio_sub_tick(A16 now,A16 hz);
int audio_drive_acquire(void);
void audio_drive_release(void);
void audio_cd_command(A16 op,A16 track);
int audio_cd_ready(void);
int audio_request(A16 op,A16 arg);
void audio_main_tick(A16 elapsed);
extern A16 audio_pending,audio_rejected,audio_link_fault;
#endif

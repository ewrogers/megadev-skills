#include "audio.h"
_Static_assert(sizeof(A16)==2 && sizeof(A32)==4,"16/32-bit integer types required");
int audio_command(AudioState *s,A16 op,A16 arg) {
    if(op==AU_PLAY){if(arg<2 || arg>3)return 0;s->desired_track=arg;s->desired_pause=0;}
    else if(op==AU_PAUSE){if(s->desired_track)s->desired_pause^=1;}
    else if(op==AU_STOP){s->desired_track=0;s->desired_pause=0;}
    else if(op==AU_BED){s->bed^=1;}
    else if(op==AU_PAN){if(arg>2)return 0;s->pan=arg;}
    else if(op==AU_PANIC){s->desired_track=0;s->desired_pause=0;s->bed=0;for(A16 i=0;i<2;i++)s->voices[i].remaining=0;s->ended_mask=3;}
    else return 0;
    return 1;
}
int audio_voice(AudioState *s,A16 sample,A16 priority,A16 ticks,A16 pan) {
    if(sample>2 || !priority || !ticks || pan>2)return -1;
    A16 which=0;
    if(!s->voices[0].remaining)which=0;
    else if(!s->voices[1].remaining)which=1;
    else {which=s->voices[1].priority<s->voices[0].priority?1:0;
        if(priority<s->voices[which].priority){s->dropped++;return -1;}}
    s->voices[which]=(AudioVoice){ticks,priority,sample,pan};return which;
}
void audio_tick(AudioState *s,A16 elapsed) {
    s->ended_mask=0;
    for(A16 i=0;i<2;i++)if(s->voices[i].remaining) {
        if(elapsed>=s->voices[i].remaining){s->voices[i].remaining=0;s->ended_mask|=(A16)(1u<<i);}
        else s->voices[i].remaining-=elapsed;
    }
}
A16 audio_drive_next(AudioState *s) {
    if(s->fault || s->inflight)return CD_NONE;
    A16 next=CD_NONE;
    if(s->data_lock || !s->desired_track){if(s->applied_track)next=CD_STOP;}
    else if(s->applied_track!=s->desired_track){next=CD_PLAY;s->pending_track=s->desired_track;}
    else if(s->applied_pause!=s->desired_pause)next=s->desired_pause?CD_PAUSE:CD_RESUME;
    s->inflight=next;return next;
}
void audio_drive_complete(AudioState *s) {
    if(s->inflight==CD_STOP){s->applied_track=0;s->applied_pause=0;}
    if(s->inflight==CD_PLAY){s->applied_track=s->pending_track;s->applied_pause=0;}
    if(s->inflight==CD_PAUSE)s->applied_pause=1;
    if(s->inflight==CD_RESUME)s->applied_pause=0;
    s->inflight=0;
}

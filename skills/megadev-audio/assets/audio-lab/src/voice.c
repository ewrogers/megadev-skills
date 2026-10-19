#include "voice.h"
int voice_begin(VoiceStream *s,A16 blocks) {
    if(!blocks || (s->phase!=VS_IDLE && s->phase!=VS_DONE && s->phase!=VS_FAULT)) {
        s->rejected++;return 0;
    }
    *s=(VoiceStream){.phase=VS_BUFFER,.total=blocks,.current=2};return 1;
}
int voice_target(VoiceStream const *s) {
    if((s->phase!=VS_BUFFER && s->phase!=VS_PLAY) || s->next>=s->total)return -1;
    A16 h=s->next&1u;
    return s->ready[h] || s->current==h?-1:(int)h;
}
int voice_commit(VoiceStream *s,A16 half) {
    if(half>1 || voice_target(s)!=(int)half)return 0;
    s->block[half]=s->next++;s->ready[half]=1;return 1;
}
void voice_observe(VoiceStream *s,A16 half) {
    if(s->phase!=VS_PLAY || half==s->current)return;
    if(half>2){s->phase=VS_FAULT;return;}
    if(s->current<2) {s->ready[s->current]=0;s->completed++;}
    s->current=half;
    if(half==2) {
        if(s->completed==s->total)s->phase=VS_DONE;
        else {s->underruns++;s->phase=VS_BUFFER;}
    } else if(!s->ready[half] || s->block[half]!=s->completed)s->phase=VS_FAULT;
}
int voice_resume(VoiceStream *s) {
    A16 h=s->completed&1u;
    if(s->phase!=VS_BUFFER || !s->ready[h] || s->block[h]!=s->completed)return -1;
    /* Two chunks before starting, except for the final chunk of a clip. */
    if(s->completed+1u<s->total && !s->ready[h^1u])return -1;
    s->current=h;s->phase=VS_PLAY;return (int)h;
}
A16 voice_loop(VoiceStream const *s) {
    if(s->phase==VS_PLAY && s->current<2) {
        A16 h=s->current^1u;
        if(s->ready[h] && s->block[h]==s->completed+1u)return h?0x8000u:0x4000u;
    }
    return VS_SILENCE;
}

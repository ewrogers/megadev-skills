#include "speech.h"
int speech_want(Speech *s,Ui16 clip,Ui16 generation) {
    if(clip>2 || !generation || generation==65535u)return 0;
    s->wanted=clip;s->generation=generation;return 1;
}
Ui16 speech_next(Speech const *s) {
    if(s->fault || s->inflight)return 0;
    if(s->applied && (s->applied!=s->wanted || s->applied_generation!=s->generation))return SPEECH_STOP;
    return !s->applied && s->wanted?SPEECH_PLAY:0;
}
int speech_sent(Speech *s,Ui16 op) {
    if(!op || op!=speech_next(s))return 0;
    s->op=op;s->arg=op==SPEECH_PLAY?s->wanted:0;s->issued_generation=s->generation;s->inflight=1;return 1;
}
void speech_complete(Speech *s,Ui16 failed) {
    if(!s->inflight)return;
    if(failed){s->fault=1;return;}
    s->applied=s->op==SPEECH_PLAY?s->arg:0;s->applied_generation=s->issued_generation;s->inflight=0;
}

#include "voice.h"
#include <assert.h>
int main(void) {
    VoiceStream s={0};assert(!voice_begin(&s,0));assert(voice_begin(&s,5));
    assert(!voice_begin(&s,4));assert(voice_target(&s)==0);
    assert(!voice_commit(&s,1));assert(voice_commit(&s,0));assert(voice_resume(&s)==-1);
    assert(voice_target(&s)==1);assert(voice_commit(&s,1));assert(voice_resume(&s)==0);
    assert(voice_loop(&s)==0x8000);assert(voice_target(&s)==-1);
    voice_observe(&s,0);assert(!s.completed);
    voice_observe(&s,1);assert(s.completed==1 && !s.ready[0]);
    assert(voice_loop(&s)==VS_SILENCE && voice_target(&s)==0);
    assert(voice_commit(&s,0));assert(voice_loop(&s)==0x4000);
    voice_observe(&s,0);assert(s.completed==2);
    /* A late refill lands at silence, never claims an unready chunk. */
    voice_observe(&s,2);assert(s.phase==VS_BUFFER && s.completed==3 && s.underruns==1);
    assert(voice_commit(&s,1));assert(voice_resume(&s)==-1);
    assert(voice_commit(&s,0));assert(voice_resume(&s)==1);
    voice_observe(&s,0);assert(s.completed==4 && voice_loop(&s)==VS_SILENCE);
    voice_observe(&s,2);assert(s.phase==VS_DONE && s.completed==5);
    assert(voice_target(&s)==-1 && voice_resume(&s)==-1);
    assert(voice_begin(&s,1));assert(voice_commit(&s,0));assert(voice_resume(&s)==0);
    assert(voice_loop(&s)==VS_SILENCE);voice_observe(&s,3);assert(s.phase==VS_FAULT);
    assert(!voice_commit(&s,1));
    /* An impossible playhead/sequence fails closed instead of freeing a live half. */
    assert(voice_begin(&s,3));assert(voice_commit(&s,0));assert(voice_commit(&s,1));
    assert(voice_resume(&s)==0);s.block[1]=9;voice_observe(&s,1);assert(s.phase==VS_FAULT);
    return 0;
}

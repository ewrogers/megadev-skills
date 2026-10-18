#include "audio.h"
#include <assert.h>
#include <string.h>
int main(void) {
    AudioState s={0};
    assert(!audio_command(&s,AU_PLAY,1));assert(!audio_command(&s,AU_PLAY,4));assert(!audio_command(&s,AU_PAN,3));
    assert(audio_command(&s,AU_PLAY,2));assert(audio_drive_next(&s)==CD_PLAY);assert(s.pending_track==2);
    assert(audio_drive_next(&s)==CD_NONE);audio_drive_complete(&s);assert(s.applied_track==2);
    assert(audio_command(&s,AU_PAUSE,0));assert(audio_drive_next(&s)==CD_PAUSE);audio_drive_complete(&s);assert(s.applied_pause);
    assert(audio_command(&s,AU_PAUSE,0));assert(audio_drive_next(&s)==CD_RESUME);audio_drive_complete(&s);assert(!s.applied_pause);
    s.data_lock=1;assert(audio_drive_next(&s)==CD_STOP);audio_drive_complete(&s);
    assert(audio_command(&s,AU_PLAY,3));assert(audio_drive_next(&s)==CD_NONE);assert(!s.applied_track);
    s.data_lock=0;assert(audio_drive_next(&s)==CD_PLAY);assert(s.pending_track==3);audio_drive_complete(&s);
    assert(s.applied_track==3);
    assert(audio_voice(&s,0,1,60,1)==0);assert(audio_voice(&s,1,3,120,2)==1);
    assert(audio_voice(&s,2,2,30,0)==0);assert(s.voices[0].sample==2);
    assert(audio_voice(&s,0,1,99,1)==-1);assert(s.dropped==1);
    audio_tick(&s,29);assert(s.voices[0].remaining==1 && s.voices[1].remaining==91);
    audio_tick(&s,1);assert(s.ended_mask==1 && !s.voices[0].remaining);
    assert(audio_voice(&s,0,1,10,1)==0);audio_tick(&s,65535);assert(s.ended_mask==3);
    assert(audio_voice(&s,3,1,1,0)==-1);assert(audio_voice(&s,0,0,1,0)==-1);assert(audio_voice(&s,0,1,0,0)==-1);
    for(unsigned hz=50;hz<=60;hz+=10) {
        assert(audio_voice(&s,0,1,300,1)==0);
        for(unsigned i=1;i<hz;i++){audio_tick(&s,(A16)(300/hz));assert(s.voices[0].remaining);}
        audio_tick(&s,(A16)(300/hz));assert(!s.voices[0].remaining);
    }
    assert(audio_command(&s,AU_BED,0) && s.bed);assert(audio_command(&s,AU_PANIC,0));
    assert(!s.bed && !s.desired_track && s.ended_mask==3);
    s.fault=1;s.desired_track=2;assert(audio_drive_next(&s)==CD_NONE);
    return 0;
}

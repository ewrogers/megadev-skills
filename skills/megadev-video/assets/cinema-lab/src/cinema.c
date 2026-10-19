#include "cinema.h"
V16 ci_word(volatile V8 const *p){return (V16)((p[0]<<8)|p[1]);}
V16 ci_sum(volatile V8 const *p,V16 size){V16 n=0;for(V16 i=0;i+1u<size;i+=2)n=(V16)(n+ci_word(p+i));return n;}
V16 ci_header(volatile V8 const *p,V32 length,V16 first){
 if(!p || length!=CI_FILE_BYTES || first>=CI_FRAMES || (first&(CI_GROUP-1u)))return 0;
 if(p[0]!='C'||p[1]!='I'||p[2]!='N'||p[3]!='1')return 0;
 return ci_word(p+4)==1 && ci_word(p+6)==CI_WIDTH && ci_word(p+8)==CI_HEIGHT &&
 ci_word(p+10)==first && ci_word(p+12)==CI_GROUP && ci_word(p+14)==CI_BYTES &&
 ci_word(p+16)==CI_PERIOD && ci_word(p+18)==CI_FRAMES;
}
void ci_init(Cinema *s){s->frame=0;s->visible=0xffffu;s->uploaded=0;s->slot=0;s->clock=0;s->late=0;s->presented=0;s->done=0;s->paused=0;}
V16 ci_begin(Cinema *s,V16 frame){
 if((s->visible==0xffffu && frame!=0) || frame>=CI_FRAMES || s->done || s->uploaded || (s->visible!=0xffffu && frame!=s->visible+1u))return 0;
 s->frame=frame;return 1;
}
V16 ci_upload(Cinema *s,V16 bytes){
 if(!bytes || (bytes&1u) || bytes>CI_BATCH || s->uploaded>CI_BYTES || bytes>CI_BYTES-s->uploaded)return 0;
 s->uploaded=(V16)(s->uploaded+bytes);return 1;
}
V16 ci_due(Cinema const *s){return !s->paused && s->uploaded==CI_BYTES && (s->visible==0xffffu || s->clock>=CI_PERIOD);}
V16 ci_present(Cinema *s){
 if(!ci_due(s))return 0;
 /* Reset to the actual presentation. Never rush a burst of frames after a stall. */
 if(s->visible!=0xffffu && s->clock>CI_PERIOD)s->late++;
 s->visible=s->frame;s->slot^=1u;s->uploaded=0;s->clock=0;s->presented++;
 if(s->frame+1u==CI_FRAMES)s->done=1;
 return 1;
}
void ci_tick(Cinema *s,V16 elapsed){
 if(s->paused || s->done)return;
 if(elapsed>300u)elapsed=300u;
 s->clock=elapsed>=300u-s->clock?300u:(V16)(s->clock+elapsed);
}

#include "cinema.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Cinema contract failed at %d\n",__LINE__);return 1;}}while(0)
int main(void){
 Cinema s;ci_init(&s);CHECK(s.visible==65535 && !ci_due(&s));
 CHECK(!ci_begin(&s,CI_FRAMES));CHECK(ci_begin(&s,0));
 CHECK(!ci_upload(&s,0));CHECK(!ci_upload(&s,3));CHECK(!ci_upload(&s,CI_BATCH+2));
 for(unsigned i=0;i<3;i++)CHECK(ci_upload(&s,2048));CHECK(ci_upload(&s,1536));
 CHECK(!ci_upload(&s,2));CHECK(ci_due(&s));CHECK(ci_present(&s));CHECK(s.visible==0 && s.slot==0);
 CHECK(!ci_begin(&s,2));CHECK(ci_begin(&s,1));
 for(unsigned i=0;i<3;i++)CHECK(ci_upload(&s,2048));CHECK(ci_upload(&s,1536));
 CHECK(!ci_due(&s));s.paused=1;ci_tick(&s,300);CHECK(!s.clock && !ci_present(&s));
 s.paused=0;ci_tick(&s,29);CHECK(!ci_due(&s));ci_tick(&s,1);CHECK(ci_due(&s));CHECK(ci_present(&s));
 CHECK(s.visible==1 && !s.clock && !s.late);ci_tick(&s,999);CHECK(s.clock==300);
 for(unsigned n=2;n<48;n++){
  CHECK(ci_begin(&s,n));CHECK(ci_upload(&s,2048));CHECK(ci_upload(&s,2048));CHECK(ci_upload(&s,2048));CHECK(ci_upload(&s,1536));
  ci_tick(&s,36);CHECK(ci_present(&s));
 }
 CHECK(s.done && s.presented==48 && s.visible==47 && s.late==46);CHECK(!ci_begin(&s,0));
 printf("Cinema contracts: %u frames; bounded uploads, pause and late holds pass\n",s.presented);return 0;
}

#include "replay.h"
#include <stdio.h>
#include <string.h>
static Replay a,b;
static int verify(int good,char const *message){if(!good)fprintf(stderr,"%s\n",message);return good;}
#define CHECK(x) do {if(!verify((x),#x))return 2;}while(0)
int main(int argc,char **argv){
    int inject=argc>1 && !strcmp(argv[1],"--inject-bug");
    rp_init(&a,0xACE1);rp_record(&a);
    for(Rp16 i=0;i<240;i++)rp_tick(&a,RP_RIGHT,rp_fixture_event(i),0);
    rp_stop(&a);RpState end=a.state;b=a;
    CHECK(a.length==240 && end.gate==1 && end.goals>0);
    CHECK(rp_play(&a));
    for(Rp16 i=0;i<240;i++)CHECK(rp_advance(&a,5,0,(Rp16)inject));
    if(inject){CHECK(a.mode==RP_DIVERGED && a.first_bad==0);printf("{\"passed\":false,\"first_divergent_tick\":0,\"intentional_bug\":true,\"expected_hash\":\"%08lx\",\"actual_hash\":\"%08lx\"}\n",(unsigned long)a.expected,(unsigned long)a.actual);return 1;}
    CHECK(a.mode==RP_MATCH && rp_hash(&a.state)==rp_hash(&end));
    CHECK(rp_play(&b));
    for(Rp16 i=0;i<200;i++)CHECK(rp_advance(&b,6,0,0));
    CHECK(b.mode==RP_MATCH && rp_hash(&b.state)==rp_hash(&end));
    /* Different service completion schedule must be detected, not normalized away. */
    b=a;b.tape[180].event=0;CHECK(rp_play(&b));
    for(Rp16 i=0;i<240;i++)rp_tick(&b,0,0,0);
    CHECK(b.mode==RP_DIVERGED && b.first_bad==180);
    b=a;b.schema++;RpState before=b.state;CHECK(!rp_play(&b));CHECK(rp_hash(&before)==rp_hash(&b.state));
    b=a;b.content++;CHECK(!rp_play(&b));b=a;b.length=RP_CAP+1;CHECK(!rp_play(&b));
    b=a;b.tape[239].held=0x8000;before=b.state;CHECK(!rp_play(&b));CHECK(rp_hash(&before)==rp_hash(&b.state));
    b=a;b.tape[20].event=3;CHECK(!rp_play(&b));
    rp_init(&b,0);CHECK(b.seed==1 && !rp_play(&b));rp_record(&b);
    for(Rp16 i=0;i<RP_CAP+20;i++)rp_tick(&b,RP_RIGHT,0,0);
    CHECK(b.length==RP_CAP && b.mode==RP_FULL);CHECK(rp_play(&b));
    before=b.state;CHECK(!rp_advance(&b,31,0,0));CHECK(rp_hash(&b.state)==rp_hash(&before));
    /* All hashed state fields matter; no padding bytes are compared. */
#define FIELD(name) do {RpState changed=before;changed.name^=1;CHECK(rp_hash(&changed)!=rp_hash(&before));}while(0)
    FIELD(x);FIELD(y);FIELD(tick);FIELD(seed);FIELD(beacons);FIELD(hits);FIELD(gate);FIELD(goals);FIELD(cooldown);FIELD(hazard_y);
    printf("{\"passed\":true,\"schema\":1,\"content\":19249,\"simulation_ticks\":240,\"ntsc_vblanks\":240,\"pal_vblanks\":200,\"final_hash\":\"%08lx\",\"service_divergence_tick\":180,\"capacity\":720}\n",(unsigned long)rp_hash(&end));
    return 0;
}

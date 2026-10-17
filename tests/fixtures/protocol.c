#include "input.h"
#include <assert.h>
#include <string.h>
typedef struct {InU8 reads[512];unsigned length,used,delay,writes,falls;InU8 last[2];InU8 values[512];InU16 ports[512],control[512];} Script;
static InU8 read(void *p,InU16 port) {
    Script *s=p;assert(port<2);assert(s->used<s->length);return s->reads[s->used++];
}
static void write(void *p,InU16 port,InU16 control,InU8 value) {
    Script *s=p;assert(port<2 && s->writes<512);s->values[s->writes]=value;s->ports[s->writes]=port;s->control[s->writes++]=control;
    if(!control){if((s->last[port]&64) && !(value&64))s->falls++;s->last[port]=value;}
}
static void settle(void *p){((Script *)p)->delay++;}
static void reset(Script *s){memset(s,0,sizeof(*s));s->last[0]=s->last[1]=96;}
static void add(Script *s,InU8 value){assert(s->length<512);s->reads[s->length++]=value;}
static void clocked(Script *s,InU8 const *n,unsigned count,unsigned first_clock) {
    for(unsigned i=0;i<count;i++)add(s,(InU8)(n[i]|(((first_clock^(i&1))!=0)?16:0)));
}
int main(void) {
    Script s;reset(&s);InIo io={&s,read,write,settle};
    struct {unsigned lo;InSample samples[8];unsigned hi;} guard={.lo=0x12345678,.hi=0xDEADBEEF};
    InSample *out=guard.samples;
    InU8 pad[8]={63,51,63,51,63,48,59,63};
    for(unsigned i=0;i<8;i++)add(&s,pad[i]);
    assert(in_poll_port(&io,0,out));assert(out[0].kind==IN_PAD6 && out[0].held==IN_X);
    assert(s.used==8 && s.last[0]==64 && s.delay==s.writes);
    assert(s.control[0]==1 && s.values[0]==64); /* TR must stay an input for a pad. */
    reset(&s);add(&s,63);add(&s,63);
    assert(!in_poll_port(&io,1,out) && !out[0].held && s.last[1]==64);
    reset(&s);add(&s,5);add(&s,12);
    assert(!in_poll_port(&io,0,out) && out[0].kind==IN_UNKNOWN && !out[0].held && s.last[0]==64);
    reset(&s);add(&s,0);add(&s,27);InU8 mouse[9]={11,15,15,0,1,0,2,0,3};clocked(&s,mouse,9,1);
    assert(in_poll_port(&io,0,out));assert(out[0].kind==IN_MOUSE && out[0].held==IN_A && out[0].dx==2 && out[0].dy==-3);
    assert(s.falls==1 && s.last[0]==96 && s.used==11); /* Relative motion captured once. */
    reset(&s);add(&s,0);add(&s,27);
    for(unsigned i=0;i<IN_WAIT_LIMIT;i++)add(&s,0); /* Never acknowledges TR=1. */
    assert(!in_poll_port(&io,0,out));assert(out[0].status==IN_TIMEOUT && s.used==2+IN_WAIT_LIMIT && s.last[0]==96);
    for(unsigned i=0;i<4;i++)assert(!out[i].held);
    for(unsigned cut=0;cut<9;cut++) {
        reset(&s);add(&s,0);add(&s,27);clocked(&s,mouse,cut,1);
        InU8 wrong=(cut&1)?16:0;
        for(unsigned i=0;i<IN_WAIT_LIMIT;i++)add(&s,wrong);
        assert(!in_poll_port(&io,0,out) && s.used==2+cut+IN_WAIT_LIMIT && s.last[0]==96);
        for(unsigned i=0;i<4;i++)assert(out[i].status==IN_TIMEOUT && !out[i].held);
    }
    reset(&s);add(&s,3);add(&s,15);
    InU8 team[17]={0,0,0,1,2,15,14,11,15,15,11,0,1,0,2,0,3};clocked(&s,team,17,0);
    assert(in_poll_port(&io,1,out));assert(out[0].held==(IN_UP|IN_A) && out[1].held==IN_X && out[2].dx==2 && out[3].kind==IN_NONE);
    assert(s.last[1]==96 && s.used==19 && s.falls==1);
    reset(&s);add(&s,3);add(&s,15);InU8 bad[6]={0,0,3,15,15,15};clocked(&s,bad,6,0);
    assert(!in_poll_port(&io,0,out) && out[0].status==IN_BAD_PACKET && s.last[0]==96);
    reset(&s);add(&s,3);add(&s,15);
    for(unsigned i=0;i<IN_WAIT_LIMIT;i++)add(&s,16); /* Header never acknowledges TR=0. */
    assert(!in_poll_port(&io,0,out) && out[3].status==IN_TIMEOUT && s.used==2+IN_WAIT_LIMIT && s.last[0]==96);
    /* Many individually legal delays must still obey a whole-packet limit. */
    reset(&s);add(&s,3);add(&s,15);
    InU8 slow[30]={0,0,2,2,2,2};
    for(unsigned i=0;i<30;i++) {
        for(unsigned d=0;d<10;d++)add(&s,(InU8)((i&1)?0:16));
        add(&s,(InU8)(slow[i]|((i&1)?16:0)));
    }
    assert(!in_poll_port(&io,0,out) && s.used==2+IN_PACKET_READ_BUDGET && s.last[0]==96);
    for(unsigned i=0;i<4;i++)assert(out[i].status==IN_TIMEOUT && !out[i].held);
    reset(&s);add(&s,3);add(&s,0);
    InU8 pads[4][8]={{63,51,63,51,63,48,59,63},{63,35,63,35,63,35,63,35},{63,51,63,51,63,48,62,63},{63,63,63,63,63,63,63,63}};
    for(unsigned phase=0;phase<8;phase++)for(unsigned player=0;player<4;player++)add(&s,pads[player][phase]);
    assert(in_poll_ea(&io,out));
    assert(out[0].kind==IN_PAD6 && out[0].held==IN_X);
    assert(out[1].kind==IN_PAD3 && out[1].held==IN_A);
    assert(out[2].kind==IN_PAD6 && out[2].held==IN_Z);
    assert(out[3].kind==IN_NONE);
    for(unsigned i=4;i<8;i++)assert(out[i].kind==IN_NONE);
    assert(s.last[0]==64 && s.last[1]==124 && s.used==34);
    unsigned selectors=0;
    for(unsigned i=5;i<s.writes-2;i++)if(s.ports[i]==1 && !s.control[i]){assert(s.values[i]==(12|((selectors&3)<<4)));selectors++;}
    assert(selectors==32);
    reset(&s);add(&s,3);add(&s,3);assert(!in_poll_ea(&io,out));assert(s.used==2 && s.last[1]==124);
    reset(&s);assert(!in_poll_port(&io,2,out) && !s.writes && !s.used);
    assert(guard.lo==0x12345678 && guard.hi==0xDEADBEEF);
    return 0;
}

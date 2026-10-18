#include "coord.h"
int sy_begin(SyClient *c,SyU16 token,SyU16 limit,SyU16 wire_idle,SyU16 main_owns) {
    if(c->phase!=SY_IDLE || token<2 || !limit || !wire_idle || !main_owns)return 0;
    c->phase=SY_GRANT;c->token=token;c->elapsed=0;c->limit=limit;
    c->discard=0;c->reason=0;c->result=0;return 1;
}
void sy_cancel(SyClient *c) {
    if(c->phase==SY_WAIT){c->phase=SY_DRAIN;c->discard=1;c->reason=SY_CANCELLED;}
}
int sy_poll(SyClient *c,SyU16 reply,SyU16 result,SyU16 mode,SyU16 elapsed) {
    if(c->phase==SY_IDLE || c->phase==SY_STUCK)return 0;
    if(c->phase==SY_RETIRE) {
        if(!reply && (mode&7u)==1u){c->phase=SY_IDLE;return 3;}
        return 0;
    }
    if(c->phase==SY_GRANT && (mode&7u)==2u){c->phase=SY_WAIT;c->elapsed=0;return 1;}
    if(elapsed>=c->limit-c->elapsed)c->elapsed=c->limit;
    else c->elapsed=(SyU16)(c->elapsed+elapsed);
    if(c->elapsed==c->limit) {
        if(c->phase==SY_GRANT){c->phase=SY_STUCK;c->discard=1;c->reason=SY_BAD_OWNER;}
        else if(c->phase==SY_WAIT){c->phase=SY_DRAIN;c->discard=1;c->reason=SY_TIMEOUT;}
    }
    /* A stalled foreground must not accept a reply first observed after its deadline. */
    if((c->phase==SY_WAIT || c->phase==SY_DRAIN) && reply==c->token && (mode&7u)==1u) {
        c->result=result;c->phase=SY_RETIRE;return 2;
    }
    return 0;
}
SyU16 sy_input(SyU16 seed,SyU16 index){return (SyU16)(seed+index*17u);}
SyU16 sy_output(SyU16 seed,SyU16 index){return (SyU16)(sy_input(seed,index)^0xA55Au);}
void sy_prepare(volatile SyU16 *b,SyU16 token,SyU16 seed) {
    b[0]=SY_MAGIC;b[1]=token;b[2]=SY_WORDS;b[3]=seed;
    for(SyU16 i=0;i<SY_WORDS;i++)b[SY_HEADER+i]=sy_input(seed,i);
    b[SY_GUARD]=0xCAFE;
}
SyU16 sy_verify(volatile SyU16 const *b,SyU16 token,SyU16 seed,SyU16 sum) {
    if(b[0]!=SY_MAGIC || b[1]!=token || b[2]!=SY_WORDS || b[3]!=seed || b[SY_GUARD]!=0xCAFE)return 0;
    SyU16 actual=0;
    for(SyU16 i=0;i<SY_WORDS;i++) {
        SyU16 value=b[SY_HEADER+i];if(value!=sy_output(seed,i))return 0;
        actual=(SyU16)(actual+value);
    }
    return actual==sum;
}
void sy_worker_begin(SyWorker *w,SyU16 token,SyU16 opcode,SyU16 seed,SyU16 count) {
    w->token=token;w->opcode=opcode;w->seed=seed;w->count=count;w->done=0;w->sum=0;
    w->result=SY_OK;w->phase=SY_WAIT;
    if(token<2 || opcode!=SY_BUILD || count!=SY_WORDS){w->result=SY_BAD_REQUEST;w->phase=SY_RETIRE;}
}
void sy_worker_step(SyWorker *w,volatile SyU16 *b,SyU16 sub_owns,SyU16 cancel) {
    if(w->phase!=SY_WAIT)return;
    if(!sub_owns){w->result=SY_BAD_OWNER;w->phase=SY_RETIRE;return;}
    if(cancel){w->result=SY_CANCELLED;w->phase=SY_RETIRE;return;}
    if(b[0]!=SY_MAGIC || b[1]!=w->token || b[2]!=w->count || b[3]!=w->seed || b[SY_GUARD]!=0xCAFE) {
        w->result=SY_BAD_DATA;w->phase=SY_RETIRE;return;
    }
    SyU16 end=(SyU16)(w->done+SY_CHUNK);if(end>w->count)end=w->count;
    while(w->done<end) {
        SyU16 i=w->done;
        if(b[SY_HEADER+i]!=sy_input(w->seed,i)){w->result=SY_BAD_DATA;w->phase=SY_RETIRE;return;}
        SyU16 value=sy_output(w->seed,i);b[SY_HEADER+i]=value;w->sum=(SyU16)(w->sum+value);w->done++;
    }
    if(w->done==w->count)w->phase=SY_RETIRE;
}

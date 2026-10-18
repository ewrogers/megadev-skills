#include "trace.h"
_Static_assert(sizeof(DbgWord)==2,"Trace needs 16-bit words");
_Static_assert((DBG_CAP & (DBG_CAP-1))==0,"Trace capacity must be a power of two");
_Static_assert(sizeof(DbgTrace)==212,"Update trace budget and CPU fixture together");
void dbg_clear(DbgTrace *t) { t->head=t->count=t->overwritten=t->frozen=0; }
void dbg_record(DbgTrace *t,DbgWord tick,DbgWord event,DbgWord token,DbgWord phase,DbgWord owner,DbgWord detail,DbgWord fault) {
    if(t->frozen)return;
    DbgEvent e={tick,event,token,phase,owner,detail};
    t->events[t->head]=e;t->head=(t->head+1u)&(DBG_CAP-1u);
    if(t->count<DBG_CAP)t->count++;else if(t->overwritten!=65535)t->overwritten++;
    if(fault){t->first=e;t->frozen=1;}
}
int dbg_get(DbgTrace const *t,DbgWord index,DbgEvent *out) {
    if(index>=t->count)return 0;
    *out=t->events[(t->head+DBG_CAP-t->count+index)&(DBG_CAP-1u)];return 1;
}

#include "trace.h"
#include <assert.h>
#include <string.h>
int main(void) {
    DbgTrace t={0};DbgEvent e={0};
    assert(!dbg_get(&t,0,&e));
    for(unsigned i=0;i<20;i++)dbg_record(&t,(DbgWord)i,DBG_START,7,1,1,0,0);
    assert(t.count==16 && t.overwritten==4);
    assert(dbg_get(&t,0,&e) && e.tick==4);
    assert(dbg_get(&t,15,&e) && e.tick==19);
    e.tick=999;assert(!dbg_get(&t,16,&e) && e.tick==999);
    dbg_record(&t,65535,DBG_TIMEOUT,7,3,2,6,1);
    DbgTrace frozen=t;
    dbg_record(&t,0,DBG_REPLY,7,4,1,1,1);
    assert(memcmp(&frozen,&t,sizeof t)==0);
    assert(t.first.tick==65535 && t.first.owner==2 && t.first.event==DBG_TIMEOUT);
    dbg_clear(&t);assert(!t.frozen && !t.count && !t.head && !t.overwritten);
    dbg_record(&t,0,DBG_START,8,1,1,0,0);
    assert(dbg_get(&t,0,&e) && e.token==8 && !t.frozen);
    t.overwritten=65535;t.count=16;
    dbg_record(&t,1,DBG_PHASE,8,2,2,0,0);assert(t.overwritten==65535);
    return 0;
}

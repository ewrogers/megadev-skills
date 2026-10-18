#ifndef LAB_TRACE_H
#define LAB_TRACE_H
typedef unsigned short DbgWord;
#define DBG_CAP 16
#define DBG_START 1
#define DBG_PHASE 2
#define DBG_REPLY 3
#define DBG_TIMEOUT 4
#define DBG_REJECT 5
#define DBG_GUARD 6
#define DBG_CANCEL 7
#define DBG_OWNER 8
typedef struct { DbgWord tick,event,token,phase,owner,detail; } DbgEvent;
typedef struct {
    DbgEvent events[DBG_CAP],first;
    DbgWord head,count,overwritten,frozen;
} DbgTrace;
/* One foreground writer per CPU. No interrupt writers or cross-CPU pointers. */
void dbg_clear(DbgTrace *trace);
void dbg_record(DbgTrace *trace,DbgWord tick,DbgWord event,DbgWord token,DbgWord phase,DbgWord owner,DbgWord detail,DbgWord fault);
int dbg_get(DbgTrace const *trace,DbgWord oldest_index,DbgEvent *out);
extern DbgTrace main_trace,sub_trace;
#endif

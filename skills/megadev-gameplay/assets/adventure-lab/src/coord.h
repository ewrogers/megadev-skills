#ifndef SYSTEM_COORD_H
#define SYSTEM_COORD_H
/* Project protocol, not Megadev APIs. All wire fields are aligned 16-bit words. */
typedef unsigned short SyU16;
#define SY_WORDS 256
#define SY_HEADER 4
#define SY_GUARD (SY_HEADER+SY_WORDS)
#define SY_BUFFER_WORDS (SY_GUARD+1)
#define SY_CHUNK 16
#define SY_MAGIC 0x5359
#define SY_BUILD 1
#define SY_OK 1
#define SY_CANCELLED 2
#define SY_BAD_REQUEST 3
#define SY_BAD_OWNER 4
#define SY_BAD_DATA 5
#define SY_IDLE 0
#define SY_GRANT 1
#define SY_WAIT 2
#define SY_DRAIN 3
#define SY_RETIRE 4
#define SY_STUCK 5
#define SY_TIMEOUT 6
typedef struct {SyU16 phase,token,elapsed,limit,discard,reason,result;} SyClient;
typedef struct {SyU16 phase,token,opcode,seed,count,done,sum,result;} SyWorker;
/* Events: 1 publish request, 2 matching reply (caller retires), 3 idle again. */
int sy_begin(SyClient *c,SyU16 token,SyU16 limit,SyU16 wire_idle,SyU16 main_owns);
void sy_cancel(SyClient *c);
int sy_poll(SyClient *c,SyU16 reply,SyU16 result,SyU16 mode,SyU16 elapsed);
SyU16 sy_input(SyU16 seed,SyU16 index);
SyU16 sy_output(SyU16 seed,SyU16 index);
void sy_prepare(volatile SyU16 *buffer,SyU16 token,SyU16 seed);
SyU16 sy_verify(volatile SyU16 const *buffer,SyU16 token,SyU16 seed,SyU16 sum);
void sy_worker_begin(SyWorker *w,SyU16 token,SyU16 opcode,SyU16 seed,SyU16 count);
void sy_worker_step(SyWorker *w,volatile SyU16 *buffer,SyU16 sub_owns,SyU16 cancel);
#endif

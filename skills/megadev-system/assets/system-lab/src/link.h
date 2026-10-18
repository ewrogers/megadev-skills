#ifndef SYSTEM_LINK_H
#define SYSTEM_LINK_H
#include "coord.h"
extern SyClient client;
extern SyU16 job_seed,passed,rejected,drained,verify_failed;
int sy_main_start(SyU16 profile,SyU16 slow);
void sy_main_tick(SyU16 elapsed);
void sy_main_cancel(void);
void sy_sub_tick(SyU16 now,SyU16 hz);
#endif

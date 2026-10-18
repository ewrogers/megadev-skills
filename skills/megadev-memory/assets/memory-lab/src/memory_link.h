#ifndef MEMORY_LINK_H
#define MEMORY_LINK_H
#include "memory.h"
#define MM_FILL2 1
#define MM_ENTER1 2
#define MM_FILL1 3
#define MM_SWAP1 4
#define MM_EXIT2 5
extern MView word_view;
extern M16 memory_phase,front_valid,back_ready,front_seed,front_token,back_seed,back_token;
extern M16 memory_pass,memory_errors,memory_blocked,memory_progress;
extern M16 memory_preview[64];
int memory_start(M16 command);
void memory_main_tick(M16 elapsed);
void memory_sub_tick(M16 now,M16 hz);
int memory_front_check(void);
#endif

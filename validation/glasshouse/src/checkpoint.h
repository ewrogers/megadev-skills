#ifndef CHECKPOINT_H
#define CHECKPOINT_H
#include "save.h"
enum { CP_LOAD=1,CP_SAVE=2,CP_TOKEN=1 };
/* Runtime readers use tokens >=2; startup's token 1 has already retired. */
extern Sv16 checkpoint_pending,checkpoint_result,checkpoint_media,checkpoint_fault;
extern SaveGame checkpoint_reply;
int checkpoint_request(Sv16 op,SaveGame const *game);
void checkpoint_main_tick(Sv16 elapsed);
void checkpoint_sub_tick(void);
int checkpoint_begin(Sv16 op);
void checkpoint_service(Sv16 elapsed);
#endif

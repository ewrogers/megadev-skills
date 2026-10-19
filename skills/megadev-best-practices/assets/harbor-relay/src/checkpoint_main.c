#include "checkpoint.h"
#include "contracts.h"
#include "stream.h"
#include <main/gate_arr.h>
Sv16 checkpoint_pending,checkpoint_result=SV_OK,checkpoint_media,checkpoint_fault;
SaveGame checkpoint_reply;
static Sv16 age;
int checkpoint_request(Sv16 op,SaveGame const *game) {
    if(checkpoint_pending || stream_phase || *ga_reg_comcmd0 || *ga_reg_comstat0 || (*ga_reg_memmode&7u)!=1u)return 0;
    if((op!=CP_LOAD && op!=CP_SAVE) || (op==CP_SAVE && (!game || !bp_checkpoint(game->level,game->coins,game->flags))))return 0;
    SaveGame snapshot=game?*game:(SaveGame){0};
    *ga_reg_comcmd1=op;*ga_reg_comcmd2=snapshot.level;*ga_reg_comcmd3=snapshot.coins;*ga_reg_comcmd7=snapshot.flags;
    __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=CP_TOKEN;
    checkpoint_pending=1;age=0;return 1;
}
void checkpoint_main_tick(Sv16 elapsed) {
    if(!checkpoint_pending || checkpoint_pending==3)return;
    if(elapsed>=9000u-age){checkpoint_pending=3;checkpoint_fault=1;return;}age+=elapsed;
    /* Reply commit before payload reads, then clear request; no early slot reuse. */
    Sv16 reply=*ga_reg_comstat0;
    if(checkpoint_pending==1 && reply==CP_TOKEN) {
        Sv16 status=*ga_reg_comstat1;
        checkpoint_result=status&255u;checkpoint_media=status>>8;
        checkpoint_reply=(SaveGame){*ga_reg_comstat2,*ga_reg_comstat5,*ga_reg_comstat6};
        __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=0;checkpoint_pending=2;
    } else if(checkpoint_pending==2 && !reply)checkpoint_pending=0;
}

#include "checkpoint.h"
#include "contracts.h"
#include "audio.h"
#include <sub/cdrom.h>
#include <sub/gate_arr.h>
extern Sv16 bram_init(void),bram_read(Sv16 slot,Sv8 *data),bram_write(Sv16 slot,Sv8 const *data);
Sv16 save_backend_read(Sv16 slot,Sv8 *out){return slot>1?SV_META:bram_read(slot,out);}
Sv16 save_backend_write(Sv16 slot,Sv8 const *data){return slot>1?SV_META:bram_write(slot,data);}
static Sv16 serving;
void checkpoint_sub_tick(void) {
    Sv16 token=*ga_reg_comcmd0;
    if(serving){if(!token){*ga_reg_comstat0=0;serving=0;}return;}
    if(token!=CP_TOKEN || *ga_reg_comstat0 || (*ga_reg_memmode&7u)!=1u || access_op!=CDROM_IDLE || audio_state.inflight || audio_state.data_lock)return;
    Sv16 op=*ga_reg_comcmd1;
    SaveGame game={*ga_reg_comcmd2,*ga_reg_comcmd3,*ga_reg_comcmd7};
    if(*ga_reg_comcmd0!=token)return;
    Sv16 media=0,result=SV_INVALID;SaveSummary summary={0};
    /* No format/delete/corruption operation is exposed by this integration. */
    if(op==CP_LOAD || (op==CP_SAVE && bp_checkpoint(game.level,game.coins,game.flags))) {
        media=bram_init();
        if(media!=3)result=SV_MEDIA;
        else if(op==CP_LOAD){result=save_load(&game,&summary);if((result==SV_LOADED || result==SV_RECOVERED) && !bp_checkpoint(game.level,game.coins,game.flags))result=SV_INVALID;}
        else result=save_write(&game,&summary);
    }
    *ga_reg_comstat1=(Sv16)((media<<8)|result);*ga_reg_comstat2=game.level;*ga_reg_comstat5=game.coins;*ga_reg_comstat6=game.flags;
    __asm__ volatile("" ::: "memory");*ga_reg_comstat0=CP_TOKEN;serving=1;
}

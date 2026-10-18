#include "save.h"
#include <sub/gate_arr.h>
extern Sv16 bram_init(void);
extern Sv16 bram_read(Sv16 slot,Sv8 *data);
extern Sv16 bram_write(Sv16 slot,Sv8 const *data);
Sv16 save_backend_read(Sv16 slot,Sv8 *out){return slot>1?SV_META:bram_read(slot,out);}
Sv16 save_backend_write(Sv16 slot,Sv8 const *data){return slot>1?SV_META:bram_write(slot,data);}
void save_sub_tick(void) {
    Sv16 token=*ga_reg_comcmd0;
    if(*ga_reg_comstat0){if(!token)*ga_reg_comstat0=0;return;}
    if(token<2)return;
    SaveGame game={*ga_reg_comcmd1,*ga_reg_comcmd2,*ga_reg_comcmd3};
    SaveSummary summary={0};Sv16 result=SV_INVALID,op=*ga_reg_comcmd4;
    Sv16 media=bram_init();
    if(media!=3)result=SV_MEDIA;
    else if(op==1 || op==3)result=save_load(&game,&summary);
    else if(op==2)result=save_write(&game,&summary);
    else if(op==4){result=save_damage(&summary);if(result==SV_OK)result=save_load(&game,&summary);}
    *ga_reg_comstat2=game.level;*ga_reg_comstat4=game.coins;*ga_reg_comstat5=game.flags;
    *ga_reg_comstat6=(Sv16)summary.sequence;*ga_reg_comstat7=media;*ga_reg_comstat1=result;
    __asm__ volatile("" ::: "memory");*ga_reg_comstat0=token;
}

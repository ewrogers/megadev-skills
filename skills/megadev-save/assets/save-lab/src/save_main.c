#include "save.h"
#include <main/gate_arr.h>
Sv16 save_pending,save_result=SV_EMPTY,save_media,save_sequence,save_fault;
Sv16 save_device;
SaveGame save_reply;
static Sv16 serial=1,age;
int save_request(Sv16 op,SaveGame const *game) {
    if(save_pending || *ga_reg_comcmd0 || *ga_reg_comstat0 || op<1 || op>5)return 0;
    if(save_device){save_result=cart_execute(op,game);return 1;}
    serial++;if(serial<2)serial=2;
    *ga_reg_comcmd1=game->level;*ga_reg_comcmd2=game->coins;*ga_reg_comcmd3=game->flags;*ga_reg_comcmd4=op;
    __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=serial;save_pending=1;age=0;return 1;
}
void save_main_tick(Sv16 elapsed) {
    if(!save_pending || save_pending==3)return;
    if(elapsed>=9000u-age){save_pending=3;save_fault=1;return;}age+=elapsed;
    if(save_pending==1 && *ga_reg_comstat0==serial) {
        save_result=*ga_reg_comstat1;save_media=*ga_reg_comstat7;save_sequence=*ga_reg_comstat6;
        save_reply=(SaveGame){*ga_reg_comstat2,*ga_reg_comstat4,*ga_reg_comstat5};
        __asm__ volatile("" ::: "memory");*ga_reg_comcmd0=0;save_pending=2;
    }
    if(save_pending==2 && !*ga_reg_comstat0)save_pending=0;
}

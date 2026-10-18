#include "save.h"
extern Sv16 cart_init(void);
extern Sv16 cart_read(Sv16 slot,Sv8 *data);
extern Sv16 cart_write(Sv16 slot,Sv8 const *data);
Sv16 cart_id;
static Sv16 cart_ready;
Sv16 save_backend_read(Sv16 slot,Sv8 *out){return cart_ready && slot<2?cart_read(slot,out):SV_MEDIA;}
Sv16 save_backend_write(Sv16 slot,Sv8 const *data){return cart_ready && slot<2?cart_write(slot,data):SV_MEDIA;}
Sv16 cart_execute(Sv16 op,SaveGame const *game) {
    cart_ready=0;cart_id=*(volatile Sv8 *)0x400001u;
    save_media=0;save_sequence=0;
    /* Standard size IDs only. Never dispatch a cartridge-supplied RAM handler. */
    if(cart_id<1 || cart_id>6){if(cart_id!=0 && cart_id!=255)save_media=4;return SV_MEDIA;}
    /* The BIOS can install a cartridge-provided handler at boot. Refuse that
       path even if a device later reports a standard size ID. */
    Sv16 opcode=*(volatile Sv16 *)0xFFFDAEu;
    Sv32 handler=*(volatile Sv32 *)0xFFFDB0u;
    if(opcode!=0x4EF9u || handler>=0x40000u || (handler&1u)){save_media=4;return SV_MEDIA;}
    save_media=cart_init();if(save_media!=3)return SV_MEDIA;
    cart_ready=1;SaveSummary summary={0};SaveGame next=*game;Sv16 result=SV_INVALID;
    if(op==1 || op==3)result=save_load(&next,&summary);
    else if(op==2)result=save_write(game,&summary);
    else if(op==4){result=save_damage(&summary);if(result==SV_OK)result=save_load(&next,&summary);}
    save_reply=next;save_sequence=(Sv16)summary.sequence;return result;
}

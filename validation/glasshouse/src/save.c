#include "save.h"
#include "contracts.h"
_Static_assert(sizeof(Sv8)==1 && sizeof(Sv16)==2 && sizeof(Sv32)==4,"Fixed byte/word/long sizes required");
static Sv16 word(Sv8 const *p){return (Sv16)((Sv16)p[0]<<8|p[1]);}
static Sv32 longword(Sv8 const *p){return (Sv32)word(p)<<16|word(p+2);}
static void put16(Sv8 *p,Sv16 n){p[0]=(Sv8)(n>>8);p[1]=(Sv8)n;}
static void put32(Sv8 *p,Sv32 n){put16(p,(Sv16)(n>>16));put16(p+2,(Sv16)n);}
static int valid(SaveGame const *g){return g && bp_checkpoint(g->level,g->coins,g->flags);}
Sv32 save_crc(Sv8 const *p,Sv16 n){Sv32 crc=0xFFFFFFFFu;while(n--){crc^=*p++;for(Sv16 i=0;i<8;i++)crc=(crc>>1)^((crc&1)?0xEDB88320u:0);}return crc^0xFFFFFFFFu;}
int save_encode(Sv8 *p,SaveGame const *g,Sv32 seq,Sv16 version) {
    if(!valid(g) || !seq || (version!=1 && version!=2))return 0;
    for(Sv16 i=0;i<SAVE_BYTES;i++)p[i]=0;
    static Sv8 const magic[8]={'M','D','S','A','V','E','0','1'};
    for(Sv16 i=0;i<8;i++)p[i]=magic[i];
    put16(p+8,version);put16(p+10,version==1?4:6);put32(p+12,seq);
    put16(p+16,g->level);put16(p+18,g->coins);if(version==2)put16(p+20,g->flags);
    put32(p+60,save_crc(p,60));return 1;
}
Sv16 save_decode(Sv8 const *p,SaveGame *g,Sv32 *seq,Sv16 *version) {
    static Sv8 const magic[8]={'M','D','S','A','V','E','0','1'};
    for(Sv16 i=0;i<8;i++)if(p[i]!=magic[i])return SLOT_CORRUPT;
    if(longword(p+60)!=save_crc(p,60))return SLOT_CORRUPT;
    Sv16 v=word(p+8);if(v>2)return SLOT_FUTURE;
    if(!v || word(p+10)!=(v==1?4:6) || !longword(p+12))return SLOT_CORRUPT;
    for(Sv16 i=v==1?20:22;i<60;i++)if(p[i])return SLOT_CORRUPT;
    SaveGame next={word(p+16),word(p+18),v==1?0:word(p+20)};
    if(!valid(&next))return SLOT_CORRUPT;
    *g=next;*seq=longword(p+12);*version=v;return SLOT_VALID;
}
static Sv16 scan(SaveGame games[2],SaveSummary *s) {
    Sv8 raw[SAVE_BYTES];Sv32 sequence[2]={0,0};Sv16 versions[2]={0,0},blocked=0;
    *s=(SaveSummary){0};s->best=2;
    for(Sv16 i=0;i<2;i++) {
        Sv16 code=save_backend_read(i,raw);
        s->slots[i]=code==SV_EMPTY?SLOT_EMPTY:code==SV_OK?save_decode(raw,&games[i],&sequence[i],&versions[i]):SLOT_ERROR;
        if(s->slots[i]==SLOT_FUTURE)blocked=SV_FUTURE;
        if(s->slots[i]==SLOT_ERROR)blocked=code;
        if(s->slots[i]==SLOT_VALID){s->valid_mask|=(Sv16)(1u<<i);if(s->best==2 || sequence[i]>s->sequence){s->best=i;s->sequence=sequence[i];s->old_version=versions[i]==1;}}
    }
    if(blocked)return blocked;
    if(s->valid_mask==3 && sequence[0]==sequence[1] && (games[0].level!=games[1].level || games[0].coins!=games[1].coins || games[0].flags!=games[1].flags))return SV_CONFLICT;
    return SV_OK;
}
Sv16 save_load(SaveGame *out,SaveSummary *s) {
    SaveGame games[2];Sv16 result=scan(games,s);if(result)return result;
    if(s->best==2)return s->slots[0]==SLOT_EMPTY && s->slots[1]==SLOT_EMPTY?SV_EMPTY:SV_INVALID;
    *out=games[s->best];return s->slots[s->best^1u]==SLOT_CORRUPT?SV_RECOVERED:SV_LOADED;
}
Sv16 save_write(SaveGame const *g,SaveSummary *s) {
    if(!valid(g))return SV_INVALID;
    SaveGame games[2];Sv16 result=scan(games,s);if(result)return result;
    if(s->sequence==0xFFFFFFFFu)return SV_EXHAUSTED;
    Sv16 target=s->best<2?(Sv16)(s->best^1u):s->slots[0]==SLOT_EMPTY?0:s->slots[1]==SLOT_EMPTY?1:0;
    Sv8 raw[SAVE_BYTES],check[SAVE_BYTES];save_encode(raw,g,s->sequence+1u,2);
    result=save_backend_write(target,raw);if(result)return result;
    if(save_backend_read(target,check)!=SV_OK)return SV_VERIFY;
    for(Sv16 i=0;i<SAVE_BYTES;i++)if(raw[i]!=check[i])return SV_VERIFY;
    s->sequence++;s->best=target;s->valid_mask|=(Sv16)(1u<<target);s->old_version=0;return SV_SAVED;
}
Sv16 save_damage(SaveSummary *s) {
    SaveGame games[2];Sv16 result=scan(games,s);if(result)return result;
    if(s->valid_mask!=3)return SV_INVALID; /* Keep a verified older fallback. */
    Sv8 raw[SAVE_BYTES];if(save_backend_read(s->best,raw)!=SV_OK)return SV_IO;
    raw[60]^=1;return save_backend_write(s->best,raw);
}

#include "save.h"
#include <assert.h>
#include <string.h>
static Sv8 records[2][64];
static Sv16 present[2],read_failure,write_failure,bad_readback,writes;
Sv16 save_backend_read(Sv16 slot,Sv8 *out){if(read_failure)return read_failure;if(!present[slot])return SV_EMPTY;memcpy(out,records[slot],64);if(bad_readback && writes)out[0]^=1;return SV_OK;}
Sv16 save_backend_write(Sv16 slot,Sv8 const *data){writes++;memcpy(records[slot],data,64);present[slot]=1;if(write_failure){records[slot][8]^=1;return SV_IO;}return SV_OK;}
static void put32(Sv8 *p,Sv32 x){for(unsigned i=0;i<4;i++)p[i]=(Sv8)(x>>(24-i*8));}
int main(void) {
    SaveGame a={2,17,1},b={4,31,3},out={9,999,0};SaveSummary s;
    assert(save_crc((Sv8 const *)"123456789",9)==0xCBF43926u);
    assert(save_load(&out,&s)==SV_EMPTY && out.level==9);
    assert(save_write(&a,&s)==SV_SAVED && s.best==0 && s.sequence==1);
    assert(save_write(&b,&s)==SV_SAVED && s.best==1 && s.sequence==2);
    assert(save_load(&out,&s)==SV_LOADED && out.level==4 && out.flags==3);
    assert(save_damage(&s)==SV_OK);
    assert(save_load(&out,&s)==SV_RECOVERED && out.level==2);
    assert(save_write(&b,&s)==SV_SAVED && s.sequence==2);
    write_failure=1;assert(save_write(&a,&s)==SV_IO);write_failure=0;
    assert(save_load(&out,&s)==SV_RECOVERED && out.level==4);
    read_failure=SV_IO;Sv16 count=writes;assert(save_write(&a,&s)==SV_IO && writes==count);read_failure=0;
    read_failure=SV_META;assert(save_write(&a,&s)==SV_META && writes==count);read_failure=0;
    /* Old records migrate in RAM; the next save writes v2 to the other slot. */
    present[0]=1;present[1]=0;assert(save_encode(records[0],&a,7,1));
    assert(save_load(&out,&s)==SV_LOADED && !out.flags && s.old_version);
    assert(save_write(&b,&s)==SV_SAVED && s.best==1 && records[1][9]==2 && records[0][9]==1);
    /* A valid future record or conflicting sequence blocks all writes. */
    records[0][9]=3;put32(records[0]+60,save_crc(records[0],60));count=writes;
    assert(save_write(&a,&s)==SV_FUTURE && writes==count);
    assert(save_encode(records[0],&a,8,2));assert(save_load(&out,&s)==SV_CONFLICT);
    assert(save_encode(records[0],&a,0xFFFFFFFFu,2));assert(save_write(&b,&s)==SV_EXHAUSTED && writes==count);
    /* Verify rejection, output/guard preservation and all single-bit errors. */
    Sv8 raw[66];memset(raw,0xA5,sizeof(raw));assert(save_encode(raw+1,&a,1,2));assert(raw[0]==0xA5 && raw[65]==0xA5);
    for(unsigned i=0;i<64;i++)for(unsigned bit=0;bit<8;bit++) {
        raw[i+1]^=(Sv8)(1u<<bit);Sv32 seq=99;Sv16 version=99;out=b;
        assert(save_decode(raw+1,&out,&seq,&version)==SLOT_CORRUPT && seq==99 && version==99 && out.level==b.level);
        raw[i+1]^=(Sv8)(1u<<bit);
    }
    a.level=0;Sv8 before[64];memcpy(before,raw+1,64);assert(!save_encode(raw+1,&a,1,2) && !memcmp(before,raw+1,64));a.level=2;
    present[0]=present[1]=0;bad_readback=1;writes=0;assert(save_write(&a,&s)==SV_VERIFY);
    return 0;
}

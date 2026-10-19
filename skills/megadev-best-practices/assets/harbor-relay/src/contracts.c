#include "contracts.h"
_Static_assert(sizeof(Bp8)==1 && sizeof(Bp16)==2,"Byte/word widths required");
int bp_slice(Bp16 capacity,Bp16 offset,Bp16 length){return offset<=capacity && length<=capacity-offset;}
int bp_read16(Bp8 const *bytes,Bp16 size,Bp16 offset,Bp16 *out) {
    if(!bytes || !out || !bp_slice(size,offset,2))return 0;
    *out=(Bp16)((Bp16)bytes[offset]<<8|bytes[offset+1u]);return 1;
}
int bp_elapsed(Bp16 now,Bp16 previous,Bp16 hz,Bp16 *out) {
    if(!out || (hz!=50 && hz!=60))return 0;
    Bp16 delta=(Bp16)(now-previous);
    *out=delta>=hz*30u?9000:(Bp16)(delta*(hz==50?6u:5u));return 1;
}
int bp_checkpoint(Bp16 level,Bp16 position,Bp16 flags){return (level==1 || level==2) && position>=16 && position<=280 && flags<=3;}

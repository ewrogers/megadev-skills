#include "packed.h"
IsoU16 iso_unpack(unsigned char const *src,IsoU16 size,unsigned char *out,IsoU16 capacity){
 IsoU16 at=0,written=0;
 if(!src || !out || !capacity)return 0;
 while(at<size){
  IsoU16 control=src[at++],count=(control&127u)+1u;
  if(count>capacity-written)return 0;
  if(control&128u){if(at>=size)return 0;unsigned char value=src[at++];for(IsoU16 i=0;i<count;i++)out[written++]=value;}
  else{if(count>size-at)return 0;for(IsoU16 i=0;i<count;i++)out[written++]=src[at++];}
 }
 return written==capacity;
}

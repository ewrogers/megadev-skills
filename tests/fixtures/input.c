#include "input.h"
#include <assert.h>
#include <string.h>
_Static_assert(sizeof(InSample)==10 && sizeof(InSlot)==22,"CPU fixture layout changed");
int main(void) {
    InSample s;InU8 pad[8]={63,51,63,51,63,51,63,51};
    assert(in_pad(pad,&s) && s.kind==IN_PAD3 && !s.held);
    pad[0]=62;pad[1]=3;assert(in_pad(pad,&s) && s.held==(IN_UP|IN_A|IN_START));
    pad[5]=48;pad[6]=48;pad[7]=63;
    assert(in_pad(pad,&s) && s.kind==IN_PAD6 && (s.held&0xF00)==0xF00);
    pad[6]=59;assert(in_pad(pad,&s) && (s.held&0xF00)==IN_X);
    pad[7]=51;assert(in_pad(pad,&s) && s.kind==IN_PAD3);
    pad[1]=63;assert(!in_pad(pad,&s) && !s.held && s.status==IN_ABSENT);
    InU8 mouse[6]={0,15,15,15,0,1};
    assert(in_mouse(mouse,&s) && s.kind==IN_MOUSE && s.dx==255 && s.dy==-1);
    assert(s.held==(IN_A|IN_B|IN_C|IN_START));
    mouse[0]=3;assert(in_mouse(mouse,&s) && s.dx==-1 && s.dy==255);
    mouse[2]=mouse[3]=mouse[4]=mouse[5]=0;
    assert(in_mouse(mouse,&s) && !s.dx && !s.dy);
    mouse[0]=4;assert(!in_mouse(mouse,&s) && s.kind==IN_MOUSE && s.status==IN_OVERFLOW && !s.held && !s.dx);
    mouse[0]=0;mouse[2]=16;assert(!in_mouse(mouse,&s) && s.status==IN_BAD_PACKET);
    InU8 packet[17]={0,0,0,1,2,15,14,11,15,15,11,0,1,0,2,0,3};
    InSample out[4];assert(in_team(packet,17,out));
    assert(out[0].kind==IN_PAD3 && out[0].held==(IN_UP|IN_A));
    assert(out[1].kind==IN_PAD6 && out[1].held==IN_X);
    assert(out[2].kind==IN_MOUSE && out[2].held==IN_A && out[2].dx==2 && out[2].dy==-3);
    assert(out[3].kind==IN_NONE && out[3].status==IN_ABSENT);
    for(unsigned length=0;length<17;length++) {
        assert(!in_team(packet,(InU16)length,out));
        for(unsigned i=0;i<4;i++)assert(out[i].kind==IN_NONE && out[i].status==IN_BAD_PACKET && !out[i].held);
    }
    assert(!in_team(packet,31,out));
    InU8 malformed[17];memcpy(malformed,packet,17);malformed[1]=1;assert(!in_team(malformed,17,out));
    memcpy(malformed,packet,17);malformed[4]=3;assert(!in_team(malformed,17,out));
    memcpy(malformed,packet,17);malformed[16]=16;assert(!in_team(malformed,17,out));
    memcpy(malformed,packet,17);malformed[11]=4;assert(in_team(malformed,17,out));
    assert(out[2].kind==IN_MOUSE && out[2].status==IN_OVERFLOW && out[1].held==IN_X);
    for(unsigned held=0;held<4096;held++) {
        InU8 raw[8];raw[0]=(InU8)(~held)&63;raw[1]=(InU8)(((~held>>2)&48)|3);
        raw[2]=raw[4]=raw[0];raw[3]=raw[1];raw[5]=raw[1]&48;
        raw[6]=(InU8)((((~held>>10)&1)|((~held>>8)&2)|((~held>>6)&4)|((~held>>8)&8))|48);
        raw[7]=(raw[1]&48)|15;
        assert(in_pad(raw,&s) && s.kind==IN_PAD6 && s.held==held);
        raw[5]=raw[7]=raw[1];assert(in_pad(raw,&s) && s.kind==IN_PAD3 && s.held==(held&255));
    }
    for(unsigned raw=0;raw<256;raw++)for(unsigned flags=0;flags<4;flags++) {
        InU8 m[6]={(InU8)flags,0,(InU8)(raw>>4),(InU8)(raw&15),(InU8)((255-raw)>>4),(InU8)((255-raw)&15)};
        assert(in_mouse(m,&s));
        int x=(int)raw,y=255-(int)raw;if(x && (flags&1))x-=256;if(y && (flags&2))y-=256;
        assert(s.dx==x && s.dy==-y);
    }
    for(unsigned types=0;types<256;types++) {
        InU8 n[30]={0};unsigned used=6;
        for(unsigned i=0;i<4;i++) {
            unsigned type=(types>>(i*2))&3;n[i+2]=(InU8)(type==3?15:type);
            unsigned size=type==0?2:(type==1?3:(type==2?6:0));
            for(unsigned j=0;j<size;j++)n[used++]=(InU8)(type==2?0:15);
        }
        assert(in_team(n,(InU16)used,out));
        for(unsigned i=0;i<4;i++) {
            unsigned type=(types>>(i*2))&3;unsigned kind=type==3?IN_NONE:type+1;
            assert(out[i].kind==kind && !out[i].held && !out[i].dx && !out[i].dy);
        }
    }
    InSlot slot;in_reset(&slot);s=(InSample){IN_PAD6,IN_OK,IN_START,0,0};
    in_update(&slot,&s,60);assert(slot.held==IN_START && !slot.pressed && !slot.repeat);
    s.held=0;in_update(&slot,&s,60);assert(slot.released==IN_START && !slot.pressed);
    s.held=IN_A;in_update(&slot,&s,60);assert(slot.pressed==IN_A);
    in_update(&slot,&s,60);assert(!slot.pressed && !slot.released);
    s.status=IN_TIMEOUT;in_update(&slot,&s,60);assert(slot.released==IN_A && !slot.held && !slot.initialized);
    s.status=IN_OK;s.held=IN_A;in_update(&slot,&s,60);assert(!slot.pressed && slot.held==IN_A);
    s.kind=IN_MOUSE;s.dx=200;s.dy=10;in_update(&slot,&s,60);assert(slot.x==160 && slot.y==200 && !slot.pressed);
    in_update(&slot,&s,60);assert(slot.x==312 && slot.y==210);
    s.dx=-255;s.dy=-255;in_update(&slot,&s,60);assert(slot.x==57 && slot.y==184);
    in_update(&slot,&s,60);assert(slot.x==0 && slot.y==184);
    unsigned repeats[2]={0,0};
    for(unsigned mode=0;mode<2;mode++) {
        unsigned hz=mode?50:60;in_reset(&slot);s=(InSample){IN_PAD3,IN_OK,0,0,0};in_update(&slot,&s,(InU16)hz);
        s.held=IN_RIGHT;
        for(unsigned i=0;i<hz;i++){in_update(&slot,&s,(InU16)hz);if(slot.repeat)repeats[mode]++;}
        s.held=IN_LEFT|IN_RIGHT;in_update(&slot,&s,(InU16)hz);assert(!slot.repeat && slot.held==12);
    }
    assert(repeats[0]==repeats[1] && repeats[0]==8);
    InSlot slots[8];for(unsigned i=0;i<8;i++)in_reset(slots+i);
    for(unsigned i=0;i<8;i++){slots[i].initialized=1;slots[i].pressed=IN_START;assert(in_join(slots,(InU16)i)==i+1);}
    assert(!in_join(slots,8));assert(in_join(slots,2)==3);
    s=(InSample){IN_NONE,IN_TIMEOUT,0,0,0};in_update(slots+2,&s,50);assert(slots[2].player==3 && !in_join(slots,2));
    return 0;
}

#include "input.h"
/* Project-owned protocol layer; this is not a Megadev SDK reader. */
static void write(InIo const *io,InU16 p,InU16 control,InU8 value) {
    io->write(io->context,p,control,value);io->settle(io->context);
}
static InU8 read(InIo const *io,InU16 p){return io->read(io->context,p);}
static int nibble(InIo const *io,InU16 p,InU8 clock,InU16 *budget) {
    write(io,p,0,clock);
    for(InU16 i=0;i<IN_WAIT_LIMIT && *budget;i++) {
        (*budget)--;InU8 v=read(io,p);
        if((v&16)==((clock&32)>>1))return v&15;
    }
    return -1;
}
static void clear_port(InSample slots[4],InU16 status){for(InU16 i=0;i<4;i++)in_clear(&slots[i],status);}
static int serial(InIo const *io,InU16 p,InU16 team,InSample out[4]) {
    InU8 n[30],clock=32;InU16 used=0,budget=IN_PACKET_READ_BUDGET;
    /* The probe already lowered TH. Keep that transaction: a second falling
       edge could consume relative mouse movement twice in one frame. */
    write(io,p,1,96);
    if(team) {
        for(InU16 i=0;i<6;i++) {
            clock^=32;int v=nibble(io,p,clock,&budget);
            if(v<0)goto timeout;
            n[used++]=(InU8)v;
        }
        if(n[0] || n[1])goto malformed;
        for(InU16 i=0;i<4;i++) {
            InU16 type=n[i+2],count=type==0?2:(type==1?3:(type==2?6:0));
            if(type!=0 && type!=1 && type!=2 && type!=15)goto malformed;
            for(InU16 j=0;j<count;j++) {
                clock^=32;int v=nibble(io,p,clock,&budget);
                if(v<0)goto timeout;
                n[used++]=(InU8)v;
            }
        }
        write(io,p,0,96);return in_team(n,used,out);
    }
    /* First mouse nibble is available with TR high as TH falls. */
    for(InU16 i=0;i<9;i++) {
        int v=nibble(io,p,clock,&budget);
        if(v<0)goto timeout;
        n[used++]=(InU8)v;clock^=32;
    }
    write(io,p,0,96);
    if(n[0]!=11)goto malformed_done;
    clear_port(out,IN_ABSENT);in_mouse(n+3,&out[0]);return out[0].status==IN_OK;
timeout:
    clear_port(out,IN_TIMEOUT);write(io,p,0,96);return 0;
malformed:
    write(io,p,0,96);
malformed_done:
    clear_port(out,IN_BAD_PACKET);return 0;
}
int in_poll_port(InIo const *io,InU16 p,InSample out[4]) {
    clear_port(out,IN_ABSENT);
    if(p>1)return 0;
    /* Probe with TH only output; driving TR here would hide pad C/Start. */
    write(io,p,1,64);write(io,p,0,96);
    InU8 steps[8];steps[0]=read(io,p);
    write(io,p,0,32);steps[1]=read(io,p);
    InU16 high=steps[0]&15,low=steps[1]&15;
    if((high==0 && low==11) || (high==3 && low==15))return serial(io,p,high==3,out);
    if((steps[1]&12)==0) {
        for(InU16 i=2;i<8;i++){write(io,p,0,(i&1)?0:64);steps[i]=read(io,p);}
        write(io,p,0,64);return in_pad(steps,&out[0]);
    }
    if(high!=15 || low!=15){out[0].kind=IN_UNKNOWN;out[0].status=IN_BAD_PACKET;}
    write(io,p,0,64);return 0;
}
int in_poll_ea(InIo const *io,InSample out[8]) {
    for(InU16 i=0;i<8;i++)in_clear(&out[i],IN_ABSENT);
    /* Explicit mode: this adapter owns BOTH physical ports. */
    write(io,0,1,64);write(io,1,1,127);write(io,0,0,64);
    write(io,1,0,12);InU8 first=read(io,0)&3;
    write(io,1,0,124);InU8 id=read(io,0)&3;
    if(!first || id){write(io,0,0,64);write(io,1,0,124);return 0;}
    InU8 steps[4][8];
    for(InU16 phase=0;phase<8;phase++) {
        write(io,0,0,(phase&1)?0:64);
        for(InU16 player=0;player<4;player++) {
            write(io,1,0,(InU8)(12|(player<<4)));steps[player][phase]=read(io,0);
        }
    }
    write(io,0,0,64);write(io,1,0,124);
    for(InU16 i=0;i<4;i++)in_pad(steps[i],&out[i]);
    return 1;
}

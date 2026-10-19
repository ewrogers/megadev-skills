#include "dialogue.h"
#include "speech.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static void present(Dialogue *s){d_present(s,s->generation);}
static void press(Dialogue *s,Ui16 key){d_step(s,key,0,0);}
static unsigned finish(Dialogue *s) {
    unsigned seen=0;
    for(unsigned i=0;i<40 && s->mode==D_TALK;i++) {
        seen|=1u<<s->node;present(s);press(s,UI_A);press(s,UI_A);
    }
    assert(s->mode==D_DONE);return seen;
}
int main(void) {
    DLayout l,old;Dialogue a,b;UiCanvas c,before;
    DSpan spans[]={{"HELLO ",6,1,0,0},{"GREEN",5,2,1,90},{".\nX\fY",5,1,0,0}};
    assert(d_compile(&l,spans,3));assert(l.pages==2 && l.glyph[22]=='X' && l.glyph[66]=='Y' && l.end[1]==1);
    assert(l.attr[6]==5 && l.delay[6]==99 && l.delay[11]==69);
    old=l;DSpan bad={"X",513,1,0,0};assert(!d_compile(&l,&bad,1));assert(!memcmp(&l,&old,sizeof l));
    bad.length=1;bad.palette=4;assert(!d_compile(&l,&bad,1));assert(!memcmp(&l,&old,sizeof l));
    char large[265];memset(large,'X',sizeof large);bad=(DSpan){large,264,1,0,0};assert(d_compile(&l,&bad,1));assert(l.pages==4 && l.end[3]==66);
    old=l;bad.length=265;assert(!d_compile(&l,&bad,1));assert(!memcmp(&l,&old,sizeof l));
    bad=(DSpan){"\fX",2,1,0,0};assert(!d_compile(&l,&bad,1));assert(!memcmp(&l,&old,sizeof l));
    DSpan wrap={"ABCDEFGHIJKLMNOPQRST XX",23,1,0,0};assert(d_compile(&l,&wrap,1));assert(l.glyph[22]=='X');
    d_init(&a);assert(d_start(&a,0));Dialogue saved=a;
    press(&a,UI_A);assert(a.revealed==0 && a.node==saved.node && !d_voice(&a));
    d_present(&a,(Ui16)(a.generation+1));assert(!a.ready);present(&a);assert(d_voice(&a)==1);
    b=a;
    for(unsigned i=0;i<60;i++)d_step(&a,0,0,5);
    for(unsigned i=0;i<50;i++)d_step(&b,0,0,6);
    assert(a.revealed==b.revealed && a.age==b.age && a.clock==b.clock);
    press(&a,UI_A);assert(a.revealed==a.text.end[0] && a.node==0 && !d_voice(&a));
    for(unsigned i=0;i<120;i++)d_step(&a,0,UI_A,5);
    assert(a.node==0);press(&a,UI_A);assert(a.node==1 && !a.ready);
    present(&a);press(&a,UI_DOWN);assert(!a.choice);press(&a,UI_A);press(&a,UI_DOWN);assert(a.choice==1);
    press(&a,UI_A);assert(a.node==3 && a.flags==2);
    unsigned seen=finish(&a);assert(seen&(1u<<4));assert(a.flags==2);
    d_init(&a);assert(d_start(&a,0));seen=finish(&a);assert((seen&0x17u)==0x17u && a.flags==1);
    d_init(&a);assert(d_start(&a,1));seen=finish(&a);assert((seen&0xE0u)==0xE0u);
    d_init(&a);assert(d_start(&a,0));present(&a);d_step(&a,0,0,60);press(&a,UI_START);saved=a;
    d_step(&a,UI_A,UI_A,60);assert(a.revealed==saved.revealed && a.clock==saved.clock && !d_voice(&a));
    press(&a,UI_START);assert(!d_voice(&a));press(&a,UI_B);assert(a.mode==D_HUB);
    d_present(&a,saved.generation);assert(a.mode==D_HUB && !d_voice(&a));
    press(&a,UI_UP);assert(a.focus==2);press(&a,UI_A);assert(a.mode==D_OPTIONS);press(&a,UI_B);assert(a.focus==2);
    for(Ui16 story=0;story<2;story++) {
        d_init(&a);d_start(&a,story);
        for(unsigned i=0;i<40 && a.mode==D_TALK;i++) {
            assert(d_draw(&c,&a,60));present(&a);press(&a,UI_A);assert(d_draw(&c,&a,50));
            for(unsigned j=0;j<UI_CELLS;j++)assert((c.cells[j]&2047)<UI_PATTERN_END);
            press(&a,UI_A);
        }
        assert(d_draw(&c,&a,50));
    }
    before=c;a.mode=8;assert(!d_draw(&c,&a,60));assert(!memcmp(&c,&before,sizeof c));
    Speech v={0};assert(speech_want(&v,1,10));assert(speech_next(&v)==SPEECH_PLAY);assert(speech_sent(&v,SPEECH_PLAY));
    assert(speech_want(&v,0,11));assert(!speech_next(&v));speech_complete(&v,0);assert(v.applied==1 && speech_next(&v)==SPEECH_STOP);
    assert(speech_sent(&v,SPEECH_STOP));assert(speech_want(&v,2,12));assert(!speech_next(&v));speech_complete(&v,0);
    assert(speech_next(&v)==SPEECH_PLAY);assert(speech_sent(&v,SPEECH_PLAY));assert(v.arg==2 && v.issued_generation==12);
    speech_complete(&v,1);assert(v.fault && v.inflight && !speech_next(&v));
    Speech previous=v;assert(!speech_want(&v,3,13));assert(!memcmp(&v,&previous,sizeof v));
    puts("PASS: layout, reveal, PAL/NTSC, choices, skip, focus, pause and stale voice retirement");
    return 0;
}

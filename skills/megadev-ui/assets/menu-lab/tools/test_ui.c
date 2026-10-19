#include "paint.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static void key(UiModel *m,Ui16 bits){ui_input(m,bits,0,0);}
static void finish(UiPainter *p,UiCanvas *c,Ui16 screen,UiBatch *b) {
    unsigned count=0;
    while(p->screen!=screen || p->building) {
        assert(ui_prepare(p,c,screen,b));assert(b->count<=4);
        assert(b->flip==(count==6));assert(ui_ack(p,b));count++;
    }
    assert(count==7);
}
int main(void) {
    UiModel m,a,b;UiCanvas c,before;UiBatch batch,saved;UiPainter p;
    ui_init(&m);key(&m,UI_DOWN);key(&m,UI_A);assert(m.page==UI_PACK);
    key(&m,UI_RIGHT);key(&m,UI_DOWN);assert(m.focus[UI_PACK]==4);
    key(&m,UI_A);assert(m.page==UI_DETAIL);key(&m,UI_B);
    assert(m.page==UI_PACK && m.focus[UI_PACK]==4);
    key(&m,UI_C);assert(m.page==UI_CHARACTER);key(&m,UI_B);key(&m,UI_B);
    assert(m.page==UI_TITLE && m.focus[UI_TITLE]==1);
    for(unsigned i=0;i<4;i++)assert(ui_push(&m,UI_SETTINGS));
    before.cells[0]=0;UiModel unchanged=m;assert(!ui_push(&m,UI_DETAIL));assert(!memcmp(&m,&unchanged,sizeof m));
    for(unsigned i=0;i<4;i++){assert(ui_pop(&m));}
    assert(!ui_pop(&m));
    ui_init(&m);key(&m,UI_A);key(&m,UI_A);assert(m.coins==7);
    for(unsigned i=0;i<100;i++)ui_input(&m,0,UI_A,5);
    assert(m.coins==7); /* Actions never repeat just because held. */
    key(&m,UI_START);assert(m.page==UI_PAUSE);key(&m,UI_DOWN);key(&m,UI_A);
    assert(m.page==UI_PACK);key(&m,UI_B);assert(m.focus[UI_PAUSE]==1);
    key(&m,UI_START);assert(m.page==UI_FIELD);
    ui_init(&a);ui_init(&b);key(&a,UI_DOWN);key(&b,UI_DOWN);
    for(unsigned i=0;i<60;i++)ui_input(&a,0,UI_DOWN,5);
    for(unsigned i=0;i<50;i++)ui_input(&b,0,UI_DOWN,6);
    assert(a.focus[0]==b.focus[0] && a.clock==b.clock && a.repeat_age==b.repeat_age);
    ui_input(&a,UI_UP|UI_DOWN,0,0);assert(a.focus[0]==b.focus[0]);
    ui_input(&a,0,UI_DOWN,0);ui_input(&a,0,UI_DOWN,9000);assert(a.focus[0]==(b.focus[0]+1)%4); /* One repeat after a stall. */
    memset(&c,0x55,sizeof c);before=c;
    assert(!ui_panel(&c,(UiRect){39,0,2,2},1));assert(!memcmp(&c,&before,sizeof c));
    assert(!ui_text(&c,39,0,2,0,1,"BAD"));assert(!memcmp(&c,&before,sizeof c));
    assert(!ui_meter(&c,0,0,8,9));assert(!memcmp(&c,&before,sizeof c));
    assert(ui_panel(&c,(UiRect){1,2,5,4},1));assert(c.cells[2*40+1]==ui_tile(UI_PANEL,1));
    assert(c.cells[5*40+5]==ui_tile(UI_PANEL+8,1));
    assert(ui_text(&c,0,0,2,0,1,"\1Z"));assert(c.cells[0]==ui_tile(UI_FONT+'?'-32,1));
    assert(ui_number(&c,0,0,3,0,123));assert(ui_number(&c,0,0,3,0,7));
    assert(c.cells[0]==ui_tile(UI_FONT+'0'-32,1) && c.cells[2]==ui_tile(UI_FONT+'7'-32,1));
    assert(ui_number(&c,0,0,2,0,100));assert(c.cells[0]==ui_tile(UI_FONT+'#'-32,1));
    ui_init(&m);for(unsigned face=0;face<2;face++)for(unsigned page=0;page<UI_PAGES;page++) {
        m.face=face;m.page=page;assert(ui_draw(&c,&m,60));
        for(unsigned i=0;i<UI_CELLS;i++)assert((c.cells[i]&2047)<UI_PATTERN_END);
    }
    before=c;m.focus[UI_TITLE]=4;assert(!ui_draw(&c,&m,60));assert(!memcmp(&c,&before,sizeof c));
    unchanged=m;key(&m,UI_A);assert(!memcmp(&m,&unchanged,sizeof m));
    ui_init(&m);assert(ui_draw(&c,&m,60));ui_painter_init(&p);
    assert(ui_prepare(&p,&c,ui_key(&m),&batch));saved=batch;
    assert(!ui_prepare(&p,&c,UI_FIELD,&batch));assert(!memcmp(&batch,&saved,sizeof batch));
    saved.epoch++;assert(!ui_ack(&p,&saved));assert(p.waiting);
    assert(ui_ack(&p,&batch));assert(p.page==0 && p.screen==65535);
    /* Cancel an incomplete hidden page: rebuild from row zero for the latest screen. */
    m.page=UI_PACK;ui_draw(&c,&m,60);
    assert(ui_prepare(&p,&c,UI_PACK,&batch));assert(batch.rows[0]==0 && !batch.flip);assert(ui_ack(&p,&batch));
    for(unsigned i=1;i<7;i++){assert(ui_prepare(&p,&c,UI_PACK,&batch));assert(batch.flip==(i==6));assert(ui_ack(&p,&batch));}
    assert(p.screen==UI_PACK && p.page==1 && !memcmp(&p.shown,&c,sizeof c));
    assert(!ui_prepare(&p,&c,UI_PACK,&batch));
    for(unsigned y=0;y<28;y++)c.cells[y*40]=ui_tile(UI_FONT+'X'-32,1);
    for(unsigned i=0;i<7;i++){assert(ui_prepare(&p,&c,UI_PACK,&batch));assert(batch.count==4 && !batch.flip && batch.page==1);assert(ui_ack(&p,&batch));}
    assert(!memcmp(&p.shown,&c,sizeof c));
    assert(!ui_prepare(&p,&c,UI_PACK,&batch));
    batch.count=1;batch.rows[0]=28;batch.page=0;batch.flip=0;batch.epoch=1;assert(!ui_batch_valid(&batch));
    batch.rows[0]=0;batch.count=2;batch.rows[1]=0;assert(!ui_batch_valid(&batch));
    ui_painter_init(&p);finish(&p,&c,UI_TITLE,&batch);
    puts("PASS: focus, repeat, layout bounds, atomic pages, immutable publication and complete dirty rows");
    return 0;
}

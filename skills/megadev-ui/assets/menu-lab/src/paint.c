#include "paint.h"
_Static_assert(sizeof(Ui16)==2 && sizeof(UiBatch)==336,"UI publication layout changed");
void ui_painter_init(UiPainter *p) {
    for(Ui16 i=0;i<UI_CELLS;i++){p->shown.cells[i]=0;p->snapshot.cells[i]=0;}
    p->screen=p->target=0xFFFFu;p->page=p->next_row=p->building=p->waiting=p->epoch=p->scan=0;
}
int ui_batch_valid(UiBatch const *b) {
    if(!b || !b->count || b->count>UI_BATCH_ROWS || b->page>1 || b->flip>1 || !b->epoch)return 0;
    for(Ui16 i=0;i<b->count;i++){
        if(b->rows[i]>=UI_ROWS)return 0;
        for(Ui16 j=0;j<i;j++)if(b->rows[j]==b->rows[i])return 0;
    }
    return 1;
}
int ui_prepare(UiPainter *p,UiCanvas const *desired,Ui16 screen,UiBatch *b) {
    if(p->waiting || screen>=UI_PAGES*2u)return 0;
    if(p->building && screen!=p->target)p->building=0;
    if(!p->building && screen!=p->screen) {
        for(Ui16 i=0;i<UI_CELLS;i++)p->snapshot.cells[i]=desired->cells[i];
        p->building=1;p->next_row=0;p->target=screen;
    }
    b->count=0;b->flip=0;b->page=p->building?(Ui16)(p->page^1u):p->page;
    if(p->building) {
        for(Ui16 y=p->next_row;y<UI_ROWS && b->count<UI_BATCH_ROWS;y++) {
            Ui16 n=b->count++;b->rows[n]=y;
            for(Ui16 x=0;x<UI_COLS;x++)b->words[n][x]=p->snapshot.cells[y*UI_COLS+x];
        }
        b->flip=p->next_row+b->count==UI_ROWS;
    } else {
        Ui16 y=p->scan;
        for(Ui16 visited=0;visited<UI_ROWS && b->count<UI_BATCH_ROWS;visited++) {
            Ui16 dirty=0;
            for(Ui16 x=0;x<UI_COLS;x++)if(p->shown.cells[y*UI_COLS+x]!=desired->cells[y*UI_COLS+x]){dirty=1;break;}
            if(dirty){Ui16 n=b->count++;b->rows[n]=y;for(Ui16 x=0;x<UI_COLS;x++)b->words[n][x]=desired->cells[y*UI_COLS+x];}
            y++;if(y==UI_ROWS)y=0;
        }p->scan=y;
    }
    if(!b->count)return 0;
    p->epoch++;if(!p->epoch)p->epoch=1;b->epoch=p->epoch;p->waiting=1;return 1;
}
int ui_ack(UiPainter *p,UiBatch const *b) {
    if(!p->waiting || !ui_batch_valid(b) || b->epoch!=p->epoch || b->page!=(p->building?(p->page^1u):p->page))return 0;
    if(p->building) {
        if(b->count>UI_ROWS-p->next_row || b->flip!=(p->next_row+b->count==UI_ROWS))return 0;
        for(Ui16 i=0;i<b->count;i++)if(b->rows[i]!=p->next_row+i)return 0;
        p->next_row+=b->count;
        if(b->flip){p->page=b->page;p->screen=p->target;p->building=0;for(Ui16 i=0;i<UI_CELLS;i++)p->shown.cells[i]=p->snapshot.cells[i];}
    } else {
        if(b->flip)return 0;
        for(Ui16 i=0;i<b->count;i++)for(Ui16 x=0;x<UI_COLS;x++)p->shown.cells[b->rows[i]*UI_COLS+x]=b->words[i][x];
    }
    p->waiting=0;return 1;
}

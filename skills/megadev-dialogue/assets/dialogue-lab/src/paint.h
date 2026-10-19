#ifndef UI_PAINT_H
#define UI_PAINT_H
#include "ui.h"
#define UI_BATCH_ROWS 4
#define UI_PAGE0 0xC000
#define UI_PAGE1 0xE000
#define UI_PLANEB 0xA000
#define UI_SAT 0xB800
#define UI_HSCROLL 0xBC00
typedef struct {Ui16 count,page,flip,epoch,rows[UI_BATCH_ROWS],words[UI_BATCH_ROWS][UI_COLS];} UiBatch;
typedef struct {
    UiCanvas shown,snapshot;
    Ui16 screen,target,page,next_row,building,waiting,epoch,scan;
} UiPainter;
void ui_painter_init(UiPainter *p);
int ui_batch_valid(UiBatch const *b);
int ui_prepare(UiPainter *p,UiCanvas const *desired,Ui16 screen,UiBatch *b);
int ui_ack(UiPainter *p,UiBatch const *b);
#endif

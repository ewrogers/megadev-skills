#ifndef GLASS_DIALOGUE_H
#define GLASS_DIALOGUE_H
#include "ui.h"
#define D_WIDTH 22
#define D_ROWS 3
#define D_PAGE 66
#define D_PAGES 4
#define D_CAP 264
#define D_END 65535u
enum { D_HUB,D_TALK,D_DONE,D_OPTIONS };
typedef struct {char const *text;Ui16 length,palette,face,pause;} DSpan;
typedef struct {Ui16 pages,end[D_PAGES];Ui8 glyph[D_CAP],attr[D_CAP];Ui16 delay[D_CAP];} DLayout;
int d_compile(DLayout *out,DSpan const *spans,Ui16 count);
#endif

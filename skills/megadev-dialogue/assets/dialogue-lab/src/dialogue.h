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
typedef struct {
    DLayout text;
    Ui16 mode,node,story,page,revealed,age,ready,generation,choice,flags,previous;
    Ui16 voice_on,cut,paused,clock,focus,repeat_key,repeat_age;
} Dialogue;
int d_compile(DLayout *out,DSpan const *spans,Ui16 count);
void d_init(Dialogue *s);
int d_start(Dialogue *s,Ui16 story);
void d_present(Dialogue *s,Ui16 generation);
void d_step(Dialogue *s,Ui16 pressed,Ui16 held,Ui16 elapsed);
Ui16 d_voice(Dialogue const *s);
int d_draw(UiCanvas *c,Dialogue const *s,Ui16 hz);
extern Dialogue dialogue;
#endif

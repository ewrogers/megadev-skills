#ifndef LANGUAGE_UI_H
#define LANGUAGE_UI_H
#include "loc.h"
typedef Lc8 Ui8;
typedef Lc16 Ui16;
#define UI_COLS 40
#define UI_ROWS 28
#define UI_CELLS 1120
#define UI_FONT 0x20
#define UI_PANEL 0xE0
#define UI_ICONS 0xF0
#define UI_PORTRAIT 0x110
#define UI_BACKDROP 0x134
#define UI_PATTERN_END 0x138
#define UI_PAGES 5
#define UI_KEYS (LC_LANGUAGES*UI_PAGES*LC_MAX_PAGES*2)
enum {UI_HOME,UI_TALK,UI_INVENTORY,UI_NAME,UI_FONTS};
enum {UI_UP=1,UI_DOWN=2,UI_LEFT=4,UI_RIGHT=8,UI_B=16,UI_C=32,UI_A=64,UI_START=128};
typedef struct {Ui16 cells[UI_CELLS];} UiCanvas;
typedef struct {Ui16 x,y,w,h;} UiRect;
typedef struct {Ui16 page,language,focus,cursor,name_length,quantity,dialog_page,paused,clock,repeat_key,repeat_age,visited;Ui8 name[LC_NAME_MAX];} UiModel;
LC_API void ui_init(UiModel *m);
LC_API void ui_input(UiModel *m,Ui16 pressed,Ui16 held,Ui16 elapsed);
LC_API Ui16 ui_key(UiModel const *m);
LC_API int ui_panel(UiCanvas *c,UiRect r,Ui16 palette);
LC_API int ui_draw(UiCanvas *c,UiModel const *m,Ui16 hz);
#endif

#ifndef MENU_UI_H
#define MENU_UI_H
typedef unsigned char Ui8;
typedef unsigned short Ui16;
#define UI_COLS 40
#define UI_ROWS 28
#define UI_CELLS 1120
#define UI_FONT 0x20
#define UI_ALT 0x80
#define UI_HEADING 0xE0
#define UI_PANEL 0x170
#define UI_ICONS 0x180
#define UI_PORTRAIT 0x1A0
#define UI_BACKDROP 0x278
#define UI_PATTERN_END 0x27C
enum { UI_TITLE,UI_FIELD,UI_PACK,UI_CHARACTER,UI_SETTINGS,UI_CREDITS,UI_PAUSE,UI_DETAIL,UI_PAGES };
enum { UI_UP=1,UI_DOWN=2,UI_LEFT=4,UI_RIGHT=8,UI_B=16,UI_C=32,UI_A=64,UI_START=128 };
typedef struct {Ui16 cells[UI_CELLS];} UiCanvas;
typedef struct {Ui16 x,y,w,h;} UiRect;
int ui_panel(UiCanvas *c,UiRect r,Ui16 palette);
int ui_text(UiCanvas *c,Ui16 x,Ui16 y,Ui16 width,Ui16 face,Ui16 palette,char const *s);
int ui_number(UiCanvas *c,Ui16 x,Ui16 y,Ui16 width,Ui16 face,Ui16 value);
int ui_meter(UiCanvas *c,Ui16 x,Ui16 y,Ui16 segments,Ui16 filled);
Ui16 ui_tile(Ui16 index,Ui16 palette);
#endif

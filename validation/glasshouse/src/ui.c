#include "ui.h"
_Static_assert(sizeof(Ui8)==1 && sizeof(Ui16)==2,"Byte/word UI types required");
Ui16 ui_tile(Ui16 index,Ui16 palette){return (Ui16)(0x8000u|(palette<<13)|index);}
static int rect(UiRect r){return r.w && r.h && r.x<UI_COLS && r.y<UI_ROWS && r.w<=UI_COLS-r.x && r.h<=UI_ROWS-r.y;}
int ui_panel(UiCanvas *c,UiRect r,Ui16 palette) {
    if(!c || !rect(r) || r.w<2 || r.h<2 || palette>3)return 0;
    for(Ui16 y=0;y<r.h;y++)for(Ui16 x=0;x<r.w;x++) {
        Ui16 sx=x==0?0:x==r.w-1?2:1,sy=y==0?0:y==r.h-1?2:1;
        c->cells[(r.y+y)*UI_COLS+r.x+x]=ui_tile((Ui16)(UI_PANEL+sy*3+sx),palette);
    }
    return 1;
}
int ui_text(UiCanvas *c,Ui16 x,Ui16 y,Ui16 width,Ui16 face,Ui16 palette,char const *s) {
    if(!c || !s || x>=UI_COLS || y>=UI_ROWS || width>UI_COLS-x || face>1 || palette>3)return 0;
    for(Ui16 i=0;i<width && s[i];i++){Ui8 ch=(Ui8)s[i];if(ch<32 || ch>=128)ch='?';c->cells[y*UI_COLS+x+i]=ui_tile((Ui16)((face?UI_ALT:UI_FONT)+ch-32),palette);}
    return 1;
}
int ui_number(UiCanvas *c,Ui16 x,Ui16 y,Ui16 width,Ui16 face,Ui16 value) {
    if(!width || width>5 || x>=UI_COLS || width>UI_COLS-x || y>=UI_ROWS || face>1)return 0;
    char buffer[6];buffer[width]=0;
    for(Ui16 i=width;i;i--){buffer[i-1]=(char)('0'+value%10u);value/=10u;}
    if(value)for(Ui16 i=0;i<width;i++)buffer[i]='#';
    return ui_text(c,x,y,width,face,1,buffer);
}
int ui_meter(UiCanvas *c,Ui16 x,Ui16 y,Ui16 segments,Ui16 filled) {
    if(!c || !segments || segments>16 || filled>segments || x>=UI_COLS || segments>UI_COLS-x || y>=UI_ROWS)return 0;
    for(Ui16 i=0;i<segments;i++)c->cells[y*UI_COLS+x+i]=ui_tile((Ui16)(UI_PANEL+(i<filled?11:12)),1);
    return 1;
}

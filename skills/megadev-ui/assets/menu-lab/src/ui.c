#include "ui.h"
_Static_assert(sizeof(Ui8)==1 && sizeof(Ui16)==2,"Byte/word UI types required");
static int model_valid(UiModel const *m) {
    if(!m || m->page>=UI_PAGES || m->face>1 || m->motion>1 || m->depth>4 || m->hp>8 || m->coins>999 || m->x>12 || m->y>5)return 0;
    if(m->focus[UI_TITLE]>=4 || m->focus[UI_PAUSE]>=4 || m->focus[UI_PACK]>=6 || m->focus[UI_SETTINGS]>=2)return 0;
    for(Ui16 i=0;i<m->depth;i++)if(m->history[i]>=UI_PAGES)return 0;
    return 1;
}
Ui16 ui_tile(Ui16 index,Ui16 palette){return (Ui16)(0x8000u|(palette<<13)|index);}
void ui_init(UiModel *m) {
    m->page=UI_TITLE;m->depth=0;m->face=0;m->motion=1;m->hp=8;m->coins=0;m->x=5;m->y=3;
    m->clock=m->repeat_key=m->repeat_age=m->badge=0;m->visited=1;
    for(Ui16 i=0;i<UI_PAGES;i++)m->focus[i]=0;
    for(Ui16 i=0;i<4;i++)m->history[i]=0;
}
int ui_push(UiModel *m,Ui16 page){if(!model_valid(m) || page>=UI_PAGES || m->depth>=4)return 0;m->history[m->depth++]=m->page;m->page=page;m->repeat_key=0;return 1;}
int ui_pop(UiModel *m){if(!model_valid(m) || !m->depth)return 0;m->page=m->history[--m->depth];m->repeat_key=0;return 1;}
static Ui16 direction(Ui16 bits){Ui16 v=bits&(UI_UP|UI_DOWN);if(v==UI_UP || v==UI_DOWN)return v;v=bits&(UI_LEFT|UI_RIGHT);return v==UI_LEFT || v==UI_RIGHT?v:0;}
static void navigate(UiModel *m,Ui16 key) {
    Ui16 page=m->page,*choice=&m->focus[page];
    if(page==UI_TITLE || page==UI_PAUSE || page==UI_SETTINGS) {
        Ui16 count=page==UI_SETTINGS?2:4;
        if(key==UI_UP)*choice=*choice?(Ui16)(*choice-1):(Ui16)(count-1);

        if(key==UI_DOWN){(*choice)++;if(*choice==count)*choice=0;}
        if(page==UI_SETTINGS && (key==UI_LEFT || key==UI_RIGHT)){if(*choice)m->motion^=1;else m->face^=1;}
    } else if(page==UI_PACK) {
        Ui16 row=*choice/3u,col=*choice-row*3u;
        if(key==UI_LEFT && col)col--;
        if(key==UI_RIGHT && col<2)col++;
        if(key==UI_UP && row)row--;
        if(key==UI_DOWN && !row)row++;
        *choice=row*3u+col;
    } else if(page==UI_FIELD) {
        if(key==UI_LEFT && m->x)m->x--;
        if(key==UI_RIGHT && m->x<12)m->x++;
        if(key==UI_UP && m->y)m->y--;
        if(key==UI_DOWN && m->y<5)m->y++;
    }
}
void ui_input(UiModel *m,Ui16 pressed,Ui16 held,Ui16 elapsed) {
    if(!model_valid(m))return;
    if(elapsed>9000)elapsed=9000;
    m->clock+=elapsed;while(m->clock>=3000)m->clock-=3000;
    Ui16 key=direction(pressed),hold=direction(held);
    if(key){navigate(m,key);m->repeat_key=key;m->repeat_age=90;}
    else if(hold && hold==m->repeat_key){if(elapsed>=m->repeat_age){Ui16 extra=(Ui16)(elapsed-m->repeat_age);navigate(m,hold);m->repeat_age=extra>=24?24:(Ui16)(24-extra);}else m->repeat_age-=elapsed;}
    else {m->repeat_key=hold;m->repeat_age=90;}
    Ui16 page=m->page;
    if(page==UI_FIELD) {
        if(pressed&UI_A)m->coins=m->coins>992?999:(Ui16)(m->coins+7);
        if(pressed&UI_B)m->hp=m->hp?(Ui16)(m->hp-1):8;
        if(pressed&UI_START)ui_push(m,UI_PAUSE);else if(pressed&UI_C)ui_push(m,UI_PACK);
    } else if(page==UI_TITLE && (pressed&(UI_A|UI_START))) {
        static Ui16 const destinations[]={UI_FIELD,UI_PACK,UI_SETTINGS,UI_CREDITS};ui_push(m,destinations[m->focus[UI_TITLE]]);
    } else if(page==UI_PAUSE && (pressed&(UI_A|UI_START))) {
        Ui16 choice=m->focus[UI_PAUSE];
        if((pressed&UI_START) || !choice)ui_pop(m);
        else if(choice==1)ui_push(m,UI_PACK);else if(choice==2)ui_push(m,UI_SETTINGS);
        else {m->page=UI_TITLE;m->depth=0;}
    } else if(pressed&(UI_B|UI_START))ui_pop(m);
    else if(page==UI_PACK){if(pressed&UI_A)ui_push(m,UI_DETAIL);else if(pressed&UI_C)ui_push(m,UI_CHARACTER);}
    else if(page==UI_SETTINGS && (pressed&UI_A)){if(m->focus[UI_SETTINGS])m->motion^=1;else m->face^=1;}
    else if(page==UI_CREDITS && (pressed&UI_A))ui_pop(m);
    m->visited|=(Ui16)(1u<<m->page);
}
Ui16 ui_key(UiModel const *m){return (Ui16)(m->page+m->face*UI_PAGES);}
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
static void icon(UiCanvas *c,Ui16 x,Ui16 y,Ui16 id) {
    if(x>=UI_COLS-1 || y>=UI_ROWS-1 || id>=8)return;
    for(Ui16 dy=0;dy<2;dy++)for(Ui16 dx=0;dx<2;dx++)c->cells[(y+dy)*UI_COLS+x+dx]=ui_tile((Ui16)(UI_ICONS+id*4+dy*2+dx),3);
}
static void label(UiCanvas *c,UiModel const *m,Ui16 x,Ui16 y,Ui16 width,char const *s,Ui16 selected) {
    Ui16 pal=selected?2:1;
    for(Ui16 i=0;i<width;i++)c->cells[y*UI_COLS+x+i]=ui_tile(m->face?UI_ALT:UI_FONT,pal);
    ui_text(c,x+2,y,(Ui16)(width-2),m->face,pal,s);
    if(selected)c->cells[y*UI_COLS+x]=ui_tile((Ui16)(UI_PANEL+9+(m->motion?((m->clock/75u)&1u):0)),pal);
}
static void heading(UiCanvas *c,Ui16 x,Ui16 y,char const *s) {
    for(Ui16 i=0;s[i] && x+1<UI_COLS && y+1<UI_ROWS;i++,x+=2) {
        Ui8 ch=(Ui8)s[i];Ui16 n=ch>='A' && ch<='Z'?(Ui16)(ch-'A'):ch>='0' && ch<='9'?(Ui16)(ch-'0'+26):0xFFFFu;
        for(Ui16 dy=0;dy<2;dy++)for(Ui16 dx=0;dx<2;dx++)c->cells[(y+dy)*UI_COLS+x+dx]=ui_tile(n==0xFFFFu?UI_FONT:(Ui16)(UI_HEADING+n*4+dy*2+dx),1);
    }
}
static void body(UiCanvas *c,UiModel const *m,Ui16 x,Ui16 y,Ui16 width,char const *s){ui_text(c,x,y,width,m->face,1,s);}
static char const * const item_names[]={"LANTERN","BRASS KEY","FIELD MAP","APPLE","INK BOTTLE","COMPASS"};
static void field(UiCanvas *c,UiModel const *m) {
    ui_panel(c,(UiRect){2,2,36,23},1);
    body(c,m,4,3,12,"FIELD NOTES");icon(c,25,3,6);ui_number(c,28,4,3,m->face,m->coins);
    body(c,m,4,5,4,"HP");ui_meter(c,8,5,8,m->hp);
    body(c,m,22,6,12,"COURIER WALK");
    for(Ui16 y=8;y<22;y++)for(Ui16 x=4;x<36;x++)c->cells[y*40+x]=ui_tile((Ui16)(UI_BACKDROP+((x+y)&1u)),0);
    icon(c,(Ui16)(5+m->x*2u),(Ui16)(9+m->y*2u),7);
    body(c,m,4,23,32,"A COINS B HP C PACK START PAUSE");
}
int ui_draw(UiCanvas *c,UiModel const *m,Ui16 hz) {
    if(!c || !model_valid(m) || (hz!=50 && hz!=60))return 0;
    for(Ui16 i=0;i<UI_CELLS;i++)c->cells[i]=0;
    Ui16 page=m->page;
    if(page==UI_FIELD || page==UI_PAUSE)field(c,m);
    else ui_panel(c,(UiRect){2,2,36,23},1);
    if(page==UI_TITLE) {
        heading(c,12,4,"MENU LAB");body(c,m,12,7,17,"WAYFARER DESK");
        static char const * const entries[]={"JOURNEY","PACK","SETTINGS","CREDITS"};
        for(Ui16 i=0;i<4;i++)label(c,m,9,(Ui16)(10+i*3u),22,entries[i],m->focus[page]==i);
        body(c,m,8,23,26,m->badge?"AUTO TOUR COMPLETE":"FONTS - PANELS - FOCUS");
    } else if(page==UI_PACK) {
        body(c,m,5,4,18,"TRAVEL PACK");body(c,m,5,6,28,"CHOOSE AN ITEM TO EXAMINE");
        for(Ui16 i=0;i<6;i++) {
            Ui16 x=(Ui16)(5+(i%3u)*11u),y=(Ui16)(9+(i/3u)*5u);
            ui_panel(c,(UiRect){x,y,8,4},m->focus[page]==i?2:1);icon(c,x+3,y+1,i);
        }
        body(c,m,5,20,30,item_names[m->focus[UI_PACK]<6?m->focus[UI_PACK]:0]);
        body(c,m,5,22,30,"A DETAILS   C CHARACTER");
    } else if(page==UI_CHARACTER) {
        body(c,m,5,4,28,"MIRA - THE WAYFARER");
        for(Ui16 y=0;y<6;y++)for(Ui16 x=0;x<6;x++)c->cells[(7+y)*40+5+x]=ui_tile((Ui16)(UI_PORTRAIT+y*6+x),3);
        body(c,m,14,7,20,"LEVEL 07");body(c,m,14,9,20,"SCOUT / COURIER");body(c,m,14,11,20,"HP");ui_meter(c,18,11,8,m->hp);
        body(c,m,5,15,29,"EQUIPMENT");icon(c,5,17,0);body(c,m,9,18,25,"TRAVEL LANTERN");icon(c,5,20,5);body(c,m,9,21,25,"BRASS COMPASS");
    } else if(page==UI_SETTINGS) {
        body(c,m,5,4,26,"DESK SETTINGS");
        label(c,m,6,9,28,"FONT FACE",m->focus[page]==0);body(c,m,10,11,22,m->face?"CLASSIC":"CLEAN");
        label(c,m,6,15,28,"CURSOR PULSE",m->focus[page]==1);body(c,m,10,17,22,m->motion?"ON":"OFF");
        body(c,m,6,21,28,"LEFT/RIGHT OR A TO CHANGE");
    } else if(page==UI_CREDITS) {
        heading(c,13,5,"THANKS");body(c,m,6,10,28,"MEGADEV MENU LAB");body(c,m,6,13,28,"ORIGINAL FONT AND UI ART");body(c,m,6,16,28,"TECHNIQUES FOR YOUR GAME");body(c,m,6,20,28,"A OR B RETURNS TO THE DESK");
    } else if(page==UI_PAUSE) {
        ui_panel(c,(UiRect){8,7,24,15},1);body(c,m,16,9,8,"PAUSED");
        static char const * const options[]={"RESUME","PACK","SETTINGS","TITLE"};
        for(Ui16 i=0;i<4;i++)label(c,m,11,(Ui16)(12+i*2u),18,options[i],m->focus[page]==i);
    } else if(page==UI_DETAIL) {
        Ui16 n=m->focus[UI_PACK]<6?m->focus[UI_PACK]:0;
        body(c,m,5,4,28,"ITEM NOTES");icon(c,6,8,n);body(c,m,11,9,24,item_names[n]);
        static char const * const description[]={"LIGHT FOR A LONG JOURNEY.","A WELL-WORN BRASS KEY.","ROADS AND PLACES TO FIND.","A SMALL SNACK FOR LATER.","INK FOR LETTERS AND MAPS.","POINTS TOWARD THE NORTH."};
        body(c,m,5,14,30,description[n]);body(c,m,5,18,30,"EXAMPLE DATA - NOT GAME RULES");
    }
    body(c,m,3,26,30,page==UI_FIELD?"D-PAD MOVE / START PAUSE":page==UI_SETTINGS?"D-PAD CHANGE   B BACK":"D-PAD CHOOSE  A OPEN  B BACK");
    ui_text(c,35,26,4,0,1,hz==50?"PAL":"NTSC");return 1;
}

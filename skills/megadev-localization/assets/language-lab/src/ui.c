#include "ui.h"
static int valid(UiModel const *m){
 if(!m || m->page>=UI_PAGES || m->language>=LC_LANGUAGES || m->focus>3 || m->cursor>=LC_NAME_COUNT+2 || m->name_length>LC_NAME_MAX || m->dialog_page>=LC_MAX_PAGES || m->paused>1)return 0;
 for(Ui16 i=0;i<m->name_length;i++)if(!m->name[i] || m->name[i]>LC_GLYPHS)return 0;
 return 1;
}
static Ui16 tile(Ui16 n,Ui16 palette){return (Ui16)(0x8000u|(palette<<13)|n);}
void ui_init(UiModel *m){
 m->page=UI_HOME;m->language=m->focus=m->cursor=m->quantity=m->dialog_page=m->paused=m->clock=m->repeat_key=m->repeat_age=0;m->visited=1;m->name_length=3;
 for(Ui16 i=0;i<LC_NAME_MAX;i++)m->name[i]=i<3?lc_default_name[i]:0;
}
static Ui16 direction(Ui16 bits){Ui16 n=bits&3;if(n==1 || n==2)return n;n=bits&12;return n==4 || n==8?n:0;}
static void nav(UiModel *m,Ui16 key){
 if(m->page==UI_HOME){
  if(key==UI_UP)m->focus=m->focus?(Ui16)(m->focus-1):3;
  if(key==UI_DOWN)m->focus=(m->focus+1)&3;
  if(key==UI_LEFT)m->language=m->language?(Ui16)(m->language-1):LC_LANGUAGES-1;
  if(key==UI_RIGHT)m->language=(m->language+1)%LC_LANGUAGES;
 }else if(m->page==UI_NAME){
  Ui16 row=m->cursor/7u,col=m->cursor%7u,n=m->cursor;
  if(key==UI_LEFT && col)n--;
  if(key==UI_RIGHT && col<6 && n+1<LC_NAME_COUNT+2)n++;
  if(key==UI_UP && row)n-=7;
  if(key==UI_DOWN && n+7<LC_NAME_COUNT+2)n+=7;
  m->cursor=n;
 }
}
static Ui16 talk_pages(UiModel const *m){Lc8 b[LC_FORMAT_CAP];LcLayout l;Ui16 n=lc_format(m->language,LC_M_STORY,m->name,m->name_length,m->quantity,b,sizeof(b));return n?lc_wrap(b,n-1,24,5,&l):0;}
void ui_input(UiModel *m,Ui16 pressed,Ui16 held,Ui16 elapsed){
 if(!valid(m))return;
 if(pressed&UI_START){m->paused^=1;m->repeat_key=m->repeat_age=0;return;}
 if(m->paused)return;
 if(elapsed>30)elapsed=30;
 m->clock+=elapsed;if(m->clock>=600)m->clock-=600;
 Ui16 key=direction(pressed),hold=direction(held);
 if(key){nav(m,key);m->repeat_key=key;m->repeat_age=90;}
 else if(hold && hold==m->repeat_key){
  if(elapsed>=m->repeat_age){Ui16 extra=elapsed-m->repeat_age;nav(m,hold);m->repeat_age=30;while(extra>=30){nav(m,hold);extra-=30;}m->repeat_age-=extra;}
  else m->repeat_age-=elapsed;
 }else{m->repeat_key=hold;m->repeat_age=90;}
 if(pressed&UI_C){m->language=(m->language+1)%LC_LANGUAGES;m->dialog_page=0;return;}
 if(pressed&UI_B){if(m->page==UI_NAME && m->name_length)m->name[--m->name_length]=0;else m->page=UI_HOME;return;}
 if(pressed&UI_A){
  if(m->page==UI_HOME){m->page=m->focus+1;m->dialog_page=0;}
  else if(m->page==UI_TALK){Ui16 pages=talk_pages(m);if(m->dialog_page+1<pages)m->dialog_page++;else m->page=UI_HOME;}
  else if(m->page==UI_INVENTORY){static Ui16 const amounts[]={0,1,2,999,65535};Ui16 i=0;while(i<4 && amounts[i]!=m->quantity)i++;i++;if(i==5)i=0;m->quantity=amounts[i];}
  else if(m->page==UI_NAME){
   if(m->cursor==LC_NAME_COUNT){if(m->name_length)m->name[--m->name_length]=0;}
   else if(m->cursor==LC_NAME_COUNT+1){if(m->name_length)m->page=UI_HOME;}
   else if(m->name_length<LC_NAME_MAX)m->name[m->name_length++]=lc_name_grid[m->cursor];
  }
 }
 m->visited|=(Ui16)(1u<<m->page);
}
Ui16 ui_key(UiModel const *m){return (Ui16)(((m->language*UI_PAGES+m->page)*LC_MAX_PAGES+m->dialog_page)*2+m->paused);}
int ui_panel(UiCanvas *c,UiRect r,Ui16 palette){
 if(!c || r.w<2 || r.h<2 || r.x>=40 || r.y>=28 || r.w>40-r.x || r.h>28-r.y || palette>3)return 0;
 for(Ui16 y=0;y<r.h;y++)for(Ui16 x=0;x<r.w;x++){Ui16 sx=!x?0:x==r.w-1?2:1,sy=!y?0:y==r.h-1?2:1;c->cells[(r.y+y)*40+r.x+x]=tile(UI_PANEL+sy*3+sx,palette);}
 return 1;
}
static void raw(UiCanvas *c,Ui16 x,Ui16 y,char const *s,Ui16 pal){while(*s && x<40){Ui8 ch=(Ui8)*s++;c->cells[y*40+x++]=tile((Ui16)(UI_FONT+(ch>=32 && ch<=126?ch-32:'?'-32)),pal);}}
static void number(UiCanvas *c,Ui16 x,Ui16 y,Ui16 n){char s[6];s[5]=0;for(Ui16 i=5;i;i--){s[i-1]=(char)('0'+n%10);n/=10;}raw(c,x,y,s,1);}
static int message(UiCanvas *c,UiModel const *m,Ui16 id,Ui16 x,Ui16 y,Ui16 width,Ui16 rows,Ui16 page,Ui16 palette){
 Lc8 b[LC_FORMAT_CAP];LcLayout l;Ui16 n=lc_format(m->language,id,m->name,m->name_length,m->quantity,b,sizeof(b));
 if(!n || !lc_wrap(b,n-1,width,rows,&l) || page>=l.pages || (id!=LC_M_STORY && l.pages>1) || x+width>40 || y+(rows-1)*2>=28)return 0;
 for(Ui16 row=0;row<rows;row++){
  Ui16 occupied=0;for(Ui16 col=0;col<width;col++)if(l.cells[(page*LC_LINES+row)*LC_WIDTH+col]!=1)occupied=1;
  if(!occupied)continue;
  for(Ui16 col=0;col<width;col++)c->cells[(y+row*2)*40+x+col]=tile(UI_FONT+l.cells[(page*LC_LINES+row)*LC_WIDTH+col]-1,palette);
 }
 return 1;
}
static void icon(UiCanvas *c,Ui16 x,Ui16 y,Ui16 n){for(Ui16 dy=0;dy<2;dy++)for(Ui16 dx=0;dx<2;dx++)c->cells[(y+dy)*40+x+dx]=tile(UI_ICONS+n*4+dy*2+dx,3);}
int ui_draw(UiCanvas *c,UiModel const *m,Ui16 hz){
 if(!c || !valid(m) || (hz!=50 && hz!=60))return 0;
 int ok=1;
 for(Ui16 i=0;i<1120;i++)c->cells[i]=tile(UI_BACKDROP,0);
 ui_panel(c,(UiRect){1,1,38,5},1);ui_panel(c,(UiRect){1,7,38,16},1);ui_panel(c,(UiRect){1,24,38,3},1);
 raw(c,3,2,"LANGUAGE LAB",1);raw(c,32,2,hz==50?"PAL":"NTSC",1);
 if(!message(c,m,LC_M_TITLE,3,4,28,1,0,1))return 0;
 Ui16 help=LC_M_BACK_HELP;
 if(m->page==UI_HOME){
  ok &= message(c,m,LC_M_LANGUAGE,23,9,14,1,0,2);
  for(Ui16 i=0;i<4;i++){raw(c,3,9+i*3,i==m->focus?">":" ",2);ok &= message(c,m,(Ui16)(LC_M_TALK+i),5,9+i*3,17,2,0,i==m->focus?2:1);}
  for(Ui16 y=0;y<6;y++)for(Ui16 x=0;x<6;x++)c->cells[(11+y)*40+27+x]=tile(UI_PORTRAIT+y*6+x,3);
  ok &= message(c,m,LC_M_WELCOME,23,19,14,2,0,1);help=LC_M_HOME_HELP;
 }else if(m->page==UI_TALK){
  ok &= message(c,m,LC_M_CLERK,3,8,30,1,0,2);
  for(Ui16 y=0;y<6;y++)for(Ui16 x=0;x<6;x++)c->cells[(10+y)*40+3+x]=tile(UI_PORTRAIT+y*6+x,3);
  if(!message(c,m,LC_M_STORY,12,10,24,5,m->dialog_page,1))return 0;
  Ui16 pages=talk_pages(m);number(c,25,21,m->dialog_page+1);raw(c,30,21,"/",1);number(c,31,21,pages);
  if(m->dialog_page+1<pages)c->cells[20*40+36]=tile(UI_PANEL+9+(m->clock/150u&1u),2);
  help=LC_M_TALK_HELP;
 }else if(m->page==UI_INVENTORY){
  ok &= message(c,m,LC_M_INVENTORY,3,8,30,1,0,2);
  for(Ui16 i=0;i<3;i++){icon(c,4,11+i*3,i==0?3:i==1?1:2);ok &= message(c,m,(Ui16)(LC_M_APPLE+i),8,11+i*3,21,1,0,1);number(c,30,11+i*3,i==0?m->quantity:i);}
  if(!message(c,m,lc_variant(m->quantity),3,21,30,1,0,1))return 0;
  help=LC_M_COUNT_HELP;
 }else if(m->page==UI_NAME){
  ok &= message(c,m,LC_M_NAME,3,8,20,1,0,2);
  for(Ui16 i=0;i<8;i++)c->cells[8*40+27+i]=tile(UI_FONT+(i<m->name_length?m->name[i]-1:'_'-32),1);
  for(Ui16 i=0;i<LC_NAME_COUNT+2;i++){
   Ui16 x=7+(i%7)*4,y=9+(i/7)*2,pal=i==m->cursor?2:1;
   raw(c,x-1,y,i==m->cursor?">":" ",pal);
   if(i<LC_NAME_COUNT)c->cells[y*40+x]=tile(UI_FONT+lc_name_grid[i]-1,pal);
   else ok &= message(c,m,i==LC_NAME_COUNT?LC_M_ERASE_KEY:LC_M_DONE_KEY,x,y,3,1,0,pal);
  }help=LC_M_NAME_HELP;
 }else{
  ok &= message(c,m,LC_M_FONTS,3,8,30,1,0,2);
  for(Ui16 i=0;i<4;i++){ok &= message(c,m,LC_M_GLYPHS+i,3,11+i*2,25,1,0,1);number(c,30,11+i*2,i==0?LC_GLYPHS:i==1?LC_GLYPHS*32:i==2?LC_FONT_SLOTS:LC_FONT_SLOTS-LC_GLYPHS);}
  ok &= message(c,m,LC_M_SAMPLE,3,21,30,1,0,1);ok &= message(c,m,LC_M_FONT_NOTE,3,19,30,1,0,1);
 }
 if(!message(c,m,help,3,25,34,1,0,1))return 0;
 if(m->paused){ui_panel(c,(UiRect){8,10,24,9},2);ok &= message(c,m,LC_M_PAUSED,11,12,18,1,0,2);ok &= message(c,m,LC_M_RESUME,11,16,18,1,0,2);}
 return ok;
}

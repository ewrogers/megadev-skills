#include "ui.h"
#include "paint.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned hash=2166136261u;
static void record(void const *p,unsigned n){unsigned char const *b=p;while(n--){hash^=*b++;hash*=16777619u;}}
int main(void){
 UiModel m;UiCanvas c;LcLayout l;Lc8 text[LC_FORMAT_CAP],guard[LC_FORMAT_CAP];
 assert(lc_language(65535)==0 && lc_variant(0)==LC_M_ZERO && lc_variant(1)==LC_M_ONE && lc_variant(2)==LC_M_MANY);
 ui_init(&m);memset(guard,0xA5,sizeof(guard));
 assert(!lc_format(0,LC_MESSAGES,m.name,3,65535,guard,sizeof(guard)));assert(guard[0]==0xA5);
 assert(!lc_format(0,LC_M_STORY,m.name,3,0,guard,2));assert(guard[0]==0xA5);
 for(unsigned lang=0;lang<LC_LANGUAGES;lang++)for(unsigned mid=0;mid<LC_MESSAGES;mid++){
  Lc16 n=lc_format(lang,mid,m.name,3,65535,text,sizeof(text));assert(n && !text[n-1]);
  assert(lc_wrap(text,n-1,24,5,&l));for(unsigned i=0;i<sizeof(l.cells);i++)assert(l.cells[i] && l.cells[i]<=LC_GLYPHS);record(text,n);record(&l,sizeof(l));
 }
 memset(&l,0xA5,sizeof(l));assert(!lc_wrap(text,1,0,5,&l));assert(l.pages==0xA5A5);
 Lc8 bad[]={253};assert(!lc_wrap(bad,1,20,5,&l));assert(l.pages==0xA5A5);
 Lc8 wide[10];memset(wide,2,sizeof(wide));assert(lc_wrap(wide,10,4,1,&l)==3);assert(l.cells[0]==2 && l.cells[LC_LINES*LC_WIDTH]==2);
 assert(!lc_wrap(wide,10,1,1,&l));
 for(unsigned lang=0;lang<LC_LANGUAGES;lang++)for(unsigned page=0;page<UI_PAGES;page++)for(unsigned pause=0;pause<2;pause++){
  ui_init(&m);m.language=lang;m.page=page;m.paused=pause;m.name_length=8;m.quantity=65535;memset(m.name,'W'-31,8);
  assert(ui_draw(&c,&m,50));record(&c,sizeof(c));assert(ui_draw(&c,&m,60));
  for(unsigned i=0;i<UI_CELLS;i++)assert((c.cells[i]&2047)<UI_PATTERN_END);
 }
 ui_init(&m);ui_input(&m,UI_A,0,0);unsigned steps=0;
 while(m.page==UI_TALK && steps++<LC_MAX_PAGES)ui_input(&m,UI_A,0,0);
 assert(m.page==UI_HOME && steps<LC_MAX_PAGES);
 ui_init(&m);ui_input(&m,UI_DOWN,UI_DOWN,0);UiModel a=m,b=m;
 for(unsigned i=0;i<60;i++)ui_input(&a,0,UI_DOWN,5);
 for(unsigned i=0;i<50;i++)ui_input(&b,0,UI_DOWN,6);
 assert(!memcmp(&a,&b,sizeof(a)));
 ui_input(&a,UI_START,0,0);b=a;ui_input(&a,UI_A|UI_C,UI_DOWN,30);assert(!memcmp(&a,&b,sizeof(a)));
 ui_init(&m);m.page=UI_NAME;for(unsigned i=0;i<20;i++)ui_input(&m,UI_A,0,0);assert(m.name_length==8);
 ui_input(&m,UI_B,0,0);assert(m.name_length==7);m.cursor=LC_NAME_COUNT+1;ui_input(&m,UI_A,0,0);assert(m.page==UI_HOME);
 ui_init(&m);m.page=UI_INVENTORY;for(unsigned i=0;i<5;i++)ui_input(&m,UI_A,0,0);assert(m.quantity==0);
 UiPainter p;UiBatch batch;ui_painter_init(&p);ui_init(&m);assert(ui_draw(&c,&m,60));UiCanvas first=c;
 for(unsigned i=0;i<7;i++){assert(ui_prepare(&p,&c,ui_key(&m),&batch));assert(batch.count==4 && batch.flip==(i==6));assert(ui_ack(&p,&batch));}
 assert(!p.building && !memcmp(&p.shown,&first,sizeof(first)));
 /* A language change gets a complete hidden page; a pending batch is immutable. */
 ui_input(&m,UI_C,0,0);assert(ui_draw(&c,&m,60));assert(ui_prepare(&p,&c,ui_key(&m),&batch));UiBatch old=batch;
 assert(!ui_prepare(&p,&first,0,&batch));assert(!memcmp(&old,&batch,sizeof(batch)));
 assert(ui_ack(&p,&batch));assert(p.building && p.screen==0);
 printf("{\"passed\":true,\"checksum\":%u,\"glyphs\":%u,\"font_bytes\":%u}\n",hash,LC_GLYPHS,LC_GLYPHS*32u);return 0;
}

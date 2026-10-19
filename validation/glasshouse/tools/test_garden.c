#include "garden.h"
#include "save.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"check failed at line %d\n",__LINE__);return 1;}}while(0)
static Sv8 records[2][64];static Sv16 exists[2];
Sv16 save_backend_read(Sv16 slot,Sv8 *out){if(!exists[slot])return SV_EMPTY;memcpy(out,records[slot],64);return SV_OK;}
Sv16 save_backend_write(Sv16 slot,Sv8 const *data){memcpy(records[slot],data,64);exists[slot]=1;return SV_OK;}
static void finish(Garden *g){for(int i=0;i<12 && g->mode==G_TALK;i++){g->ready=1;garden_step(g,5,0,64);}}
int main(void) {
 Garden n,p;garden_init(&n);CHECK(!garden_step(&n,5,0,64));
 n.ready=1;CHECK(garden_step(&n,5,0,64)==G_TRAVEL);
 garden_enter(&n,0);n.x=48;n.y=128;n.ready=1;p=n;
 for(int i=0;i<60;i++)garden_step(&n,5,8,0);
 for(int i=0;i<50;i++)garden_step(&p,6,8,0);
 CHECK(n.x==168 && n.x==p.x && n.y==p.y && n.tick==60 && p.tick==60 && !p.clock);
 garden_step(&n,5,0,128);G16 x=n.x,tick=n.tick;n.ready=1;
 for(int i=0;i<50;i++)garden_step(&n,6,8,0);
 CHECK(n.mode==G_PAUSE && n.x==x && n.tick==tick);
 n.ready=1;n.menu=1;CHECK(garden_step(&n,5,0,64)==G_SAVE);
 n.ready=1;garden_step(&n,5,0,16);CHECK(n.mode==G_FIELD);
 garden_enter(&n,0);n.ready=1;n.x=120;n.y=104;
 for(int i=0;i<30;i++)garden_step(&n,5,1,0);
 CHECK(n.y==100 && garden_position(0,n.x,n.y));
 CHECK(!garden_position(0,120,80) && !garden_position(2,48,128));
 n.progress=0;CHECK(garden_open(&n,G_CARETAKER));n.ready=1;garden_step(&n,5,0,16);CHECK(!n.progress && !n.clock);
 n.x=48;n.y=128;n.ready=1;garden_step(&n,5,8,0);CHECK(n.x==50);
 CHECK(garden_open(&n,G_VALVE));finish(&n);CHECK(!(n.progress&G_ROUTE));
 CHECK(garden_open(&n,G_ENGINE));finish(&n);CHECK(n.progress==G_PUMP);
 CHECK(garden_open(&n,G_VALVE));n.ready=1;garden_step(&n,5,0,64);n.ready=1;garden_step(&n,5,0,2);finish(&n);CHECK(!(n.progress&G_ROUTE));
 CHECK(garden_open(&n,G_VALVE));finish(&n);CHECK(n.progress==(G_PUMP|G_ROUTE));
 CHECK(garden_open(&n,G_FLOWER));finish(&n);CHECK(n.mode==G_FIELD && !(n.progress&G_BLOOM));
 CHECK(garden_open(&n,G_CARETAKER));finish(&n);CHECK(n.progress==7);
 CHECK(garden_open(&n,G_FLOWER));finish(&n);CHECK(n.progress==15 && n.mode==G_END);
 SaveGame a={1,48,(128<<4)|1},b={2,64,(128<<4)|7},out={0};SaveSummary summary;
 CHECK(save_write(&a,&summary)==SV_SAVED);CHECK(save_write(&b,&summary)==SV_SAVED);
 CHECK(save_load(&out,&summary)==SV_LOADED && out.level==2 && out.flags==b.flags);
 records[1][60]^=1;CHECK(save_load(&out,&summary)==SV_RECOVERED && out.level==1 && out.flags==a.flags);
 CHECK(!garden_save_valid(1,120,(80<<4)|1));CHECK(!garden_save_valid(2,64,(128<<4)|4));
 CHECK(!garden_save_valid(2,64,(128<<4)|8));CHECK(!garden_save_valid(3,64,128<<4));
 UiCanvas c;garden_init(&n);CHECK(garden_open(&n,G_CARETAKER));
 n.revealed=n.text.end[0];n.clock=0;garden_draw(&c,&n,60,0,0);
 CHECK((c.cells[25*40+36]&0x7FF)==0x7F);
 n.clock=200;garden_draw(&c,&n,60,0,0);CHECK((c.cells[26*40+36]&0x7FF)==0x7F);
 n.page=n.text.pages-1;n.revealed=n.text.end[n.page];garden_draw(&c,&n,60,0,0);
 CHECK((c.cells[26*40+36]&0x7FF)!=0x7F);
 garden_init(&n);
 for(G16 mode=0;mode<=G_END;mode++){n.mode=mode;garden_draw(&c,&n,60,0,0);for(int i=0;i<UI_CELLS;i++)CHECK((c.cells[i]&0x7FF)<0xC0);}
 return 0;
}

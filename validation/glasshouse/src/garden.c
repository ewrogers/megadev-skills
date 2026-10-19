#include "garden.h"
_Static_assert(sizeof(G16)==2,"16-bit game words required");
static int near(G16 a,G16 b,G16 distance){return a>b?a-b<=distance:b-a<=distance;}
void garden_init(Garden *g) {
 volatile Ui8 *bytes=(volatile Ui8 *)g;for(G16 i=0;i<sizeof(*g);i++)bytes[i]=0;g->x=48;g->y=128;g->mode=G_TITLE;g->face=1;
}
void garden_enter(Garden *g,G16 area) {
 g->area=area;g->x=area?48:264;g->y=128;g->mode=G_FIELD;g->clock=0;g->ready=0;g->node=0;g->walking=0;
}
G16 garden_screen(Garden const *g){return (G16)(1u+g->mode*64u+(g->mode==G_TALK?g->node*4u+g->page:0u));}
G16 garden_near(Garden const *g) {
 if(near(g->x,g->area?24:288,24) && near(g->y,128,20))return G_DOOR;
 if(!g->area) {
  if(near(g->x,64,24) && near(g->y,64,24))return G_CARETAKER;
  if(near(g->x,232,26) && near(g->y,112,24))return G_FLOWER;
 } else {
  if(near(g->x,64,24) && near(g->y,80,24))return G_ENGINE;
  if(near(g->x,240,26) && near(g->y,144,24))return G_VALVE;
 }
 return G_NONE;
}
G16 garden_open(Garden *g,G16 node) {
 char const *text=0;
 if(node==G_CARETAKER)text="The moonflower needs rain. Prime the pump next door, then route water to the glasshouse. Come back to tend the flower.";
 if(node==G_ENGINE)text=g->progress&G_PUMP?"The pump is running quietly. Now send the water toward the garden.":"Prime the rain pump? A gentle turn will wake it. Press A when ready.";
 if(node==G_VALVE)text=g->progress&G_PUMP?"Where should the water go? Choose a route below, then press A.":"The pressure gauge is empty. Prime the pump on the left first.";
 if(node==G_FLOWER)text=(g->progress&7u)==7u?"The soil is cool and damp. Tend the moonflower and watch its petals open.":"The moonflower is waiting. Ask the caretaker by the upper-left window what it needs.";
 if(!text)return 0;
 DSpan span={text,0,0,0,0};while(text[span.length])span.length++;
 if(!d_compile(&g->text,&span,1))return 0;
 g->node=node;g->page=g->revealed=g->age=g->choice=g->ready=g->clock=0;g->mode=G_TALK;return 1;
}
static void close_talk(Garden *g) {
 g->clock=0;
 if(g->node==G_CARETAKER)g->progress|=G_LEARNED;
 if(g->node==G_ENGINE)g->progress|=G_PUMP;
 if(g->node==G_VALVE && (g->progress&G_PUMP)) {
  if(!g->choice)g->progress|=G_ROUTE;else g->progress&=(G16)~G_ROUTE;
 }
 if(g->node==G_FLOWER && (g->progress&7u)==7u){g->progress|=G_BLOOM;g->mode=G_END;}
 else g->mode=G_FIELD;
 g->ready=0;g->sound=1;g->request|=G_EFFECT;
}
G16 garden_step(Garden *g,G16 elapsed,G16 held,G16 hit) {
 g->request=0;g->sound=0;
 if(!g->ready)return 0;
 if(elapsed>20)elapsed=20;
 if(g->mode==G_TALK) {
  g->clock+=elapsed;if(g->clock>=300)g->clock-=300;
  if(hit&16){g->mode=G_FIELD;g->ready=0;g->clock=0;return 0;}
  G16 end=g->text.end[g->page];
  if(hit&64) {
   if(g->revealed<end){g->revealed=end;g->age=0;return 0;}
   if(g->page+1<g->text.pages){g->page++;g->revealed=g->age=g->ready=0;return 0;}
   close_talk(g);return g->request;
  }
  if(g->revealed==end && g->page+1==g->text.pages && g->node==G_VALVE && g->progress&G_PUMP && hit&3)g->choice^=1;
  g->age+=elapsed;
  for(G16 i=0;i<D_PAGE && g->revealed<end;i++) {
   G16 at=(G16)(g->page*D_PAGE+g->revealed),delay=g->text.delay[at];
   if(delay>g->age)break;
   g->age-=delay;g->revealed++;
  }
  if(g->revealed==end)g->age=0;
  return 0;
 }
 if(g->mode==G_TITLE) {
  if(hit&2){g->menu++;if(g->menu==3)g->menu=0;}
  if(hit&1)g->menu=g->menu?(G16)(g->menu-1u):2;
  if(hit&(64|128)) {
   if(g->menu==0){g->progress=0;g->request=G_TRAVEL;}
   if(g->menu==1)g->request=G_RESUME;
   if(g->menu==2){g->mode=G_GUIDE;g->ready=0;}
  }return g->request;
 }
 if(g->mode==G_GUIDE || g->mode==G_END) {
  if(hit&(16|64|128)){g->mode=G_TITLE;g->ready=0;g->request=G_LEAVE;}return g->request;
 }
 if(g->mode==G_PAUSE || g->mode==G_JOURNAL) {
  if(hit&(16|128)){g->mode=G_FIELD;g->ready=0;return 0;}
  if(hit&2){g->menu++;if(g->menu==3)g->menu=0;}
  if(hit&1)g->menu=g->menu?(G16)(g->menu-1u):2;
  if((hit&64) && g->mode==G_PAUSE) {
   if(!g->menu){g->mode=G_FIELD;g->ready=0;}
   if(g->menu==1)g->request=G_SAVE;
   if(g->menu==2)g->request=G_LEAVE;
  }return g->request;
 }
 if(hit&128){g->mode=G_PAUSE;g->menu=0;g->clock=0;g->ready=0;return 0;}
 if(hit&16){g->mode=G_JOURNAL;g->menu=0;g->clock=0;g->ready=0;return 0;}
 if(hit&64) {
  G16 node=garden_near(g);
  if(node==G_DOOR)g->request=G_TRAVEL;else garden_open(g,node);
  return g->request;
 }
 g->clock+=elapsed;g->walking=0;
 while(g->clock>=5) {
  g->clock-=5;g->tick++;
  G16 x=g->x,y=g->y;
  if((held&12)==4){x-=G_WALK_STEP;g->face=2;}if((held&12)==8){x+=G_WALK_STEP;g->face=3;}
  if(garden_position(g->area,x,y))g->x=x;
  x=g->x;
  if((held&3)==1){y-=G_WALK_STEP;g->face=0;}if((held&3)==2){y+=G_WALK_STEP;g->face=1;}
  if(garden_position(g->area,x,y))g->y=y;
  g->walking=!!(held&15);
 }
 return 0;
}
static void txt(UiCanvas *c,G16 x,G16 y,G16 width,char const *s){ui_text(c,x,y,width,0,0,s);}
void garden_draw(UiCanvas *c,Garden const *g,G16 hz,G16 busy,G16 save_status) {
 for(G16 i=0;i<UI_CELLS;i++)c->cells[i]=g->mode==G_FIELD || g->mode==G_TALK?0:ui_tile(0x20,0);
 ui_panel(c,(UiRect){0,0,40,4},0);txt(c,2,1,29,"GLASSHOUSE AFTER HOURS");txt(c,35,1,4,hz==50?"PAL":"NTSC");
 txt(c,2,2,28,g->area?"RAINWORKS / SERVICE FLOOR":"CONSERVATORY / NIGHT GARDEN");
 if(g->mode==G_TITLE) {
  ui_panel(c,(UiRect){3,6,34,18},0);txt(c,7,8,28,"A QUIET NIGHT. A LAST BLOOM.");
  txt(c,7,10,27,"Restore rain to the garden");txt(c,7,11,27,"before its petals open.");
  txt(c,9,14,24,g->menu==0?"> BEGIN THE NIGHT":"  BEGIN THE NIGHT");
  txt(c,9,16,24,g->menu==1?"> CONTINUE":"  CONTINUE");txt(c,9,18,24,g->menu==2?"> HOW TO PLAY":"  HOW TO PLAY");
  txt(c,7,21,27,busy?"OPENING THE GLASSHOUSE...":"D-PAD CHOOSE / A OR START");
 } else if(g->mode==G_GUIDE) {
  ui_panel(c,(UiRect){2,6,36,19},0);txt(c,4,8,32,"YOU ARE THE EVENING GARDENER.");
  txt(c,4,11,32,"D-PAD  Walk around beds");txt(c,4,13,32,"A      Talk, inspect, use doors");txt(c,4,15,32,"B      Read your field notes");txt(c,4,17,32,"START  Pause, save, return title");
  txt(c,4,20,32,"First: talk near the north-west");txt(c,4,21,32,"window. Look for A prompts.");txt(c,4,23,32,"A / B RETURN");
 } else if(g->mode==G_END) {
  ui_panel(c,(UiRect){3,6,34,19},0);txt(c,7,9,27,"THE MOONFLOWER OPENS");txt(c,6,12,29,"Rain returns to the garden.");txt(c,6,14,29,"The night shift can rest.");
  txt(c,6,18,29,"GLASSHOUSE AFTER HOURS");txt(c,6,20,29,"An original Megadev showcase");txt(c,6,23,29,"A / START  RETURN TO TITLE");
 } else if(g->mode==G_JOURNAL || g->mode==G_PAUSE) {
  ui_panel(c,(UiRect){3,6,34,19},0);txt(c,6,8,28,g->mode==G_PAUSE?"TAKE A BREATHER":"FIELD NOTES");
  txt(c,6,10,28,g->progress&G_LEARNED?"[+] Caretaker's instructions":"[ ] Speak with the caretaker");
  txt(c,6,12,28,g->progress&G_PUMP?"[+] Rain pump primed":"[ ] Prime the pump next door");
  txt(c,6,14,28,g->progress&G_ROUTE?"[+] Water sent to the garden":"[ ] Set valve toward garden");
  if(g->mode==G_PAUSE) {
   txt(c,6,17,27,g->menu==0?"> RESUME":"  RESUME");txt(c,6,19,27,g->menu==1?"> SAVE NOTEBOOK":"  SAVE NOTEBOOK");txt(c,6,21,27,g->menu==2?"> RETURN TO TITLE":"  RETURN TO TITLE");
  } else txt(c,6,19,28,"Then tend the flower on the");
  if(g->mode==G_JOURNAL)txt(c,6,20,28,"right side of the garden.");
  txt(c,6,23,28,"B / START BACK");
 } else if(g->mode==G_TALK) {
  ui_panel(c,(UiRect){1,17,38,11},0);txt(c,3,18,34,g->node==G_CARETAKER?"ROWAN / NIGHT CARETAKER":g->node==G_ENGINE?"RAIN PUMP":g->node==G_VALVE?"FLOW VALVE":"MOONFLOWER");
  if(g->text.pages>1) {
   char count[4]={(char)('1'+g->page),'/',(char)('0'+g->text.pages),0};txt(c,33,18,3,count);
  }
  for(G16 i=0;i<g->revealed;i++) {
   G16 at=(G16)(g->page*D_PAGE+i);char ch[2]={(char)g->text.glyph[at],0};if(ch[0])txt(c,(G16)(9+i%D_WIDTH),(G16)(20+i/D_WIDTH),1,ch);
  }
  if(g->revealed==g->text.end[g->page] && g->page+1==g->text.pages && g->node==G_VALVE && g->progress&G_PUMP) {
   txt(c,9,24,24,g->choice?"  GARDEN   > DRAIN":"> GARDEN     DRAIN");
  }
  if(g->revealed==g->text.end[g->page] && g->page+1<g->text.pages)
   c->cells[(g->clock<150?25u:26u)*40u+36u]=ui_tile(0x7F,0);
  txt(c,3,26,32,g->revealed<g->text.end[g->page]?"A REVEAL / B CLOSE":g->page+1<g->text.pages?"A MORE / B CLOSE":"A CONFIRM / B CLOSE");
 } else {
  ui_panel(c,(UiRect){0,24,40,4},0);G16 node=garden_near(g);
  txt(c,2,25,36,node==G_DOOR?(g->area?"A  RETURN TO CONSERVATORY":"A  ENTER RAINWORKS"):node==G_CARETAKER?"A  SPEAK WITH ROWAN":node==G_ENGINE?"A  INSPECT RAIN PUMP":node==G_VALVE?"A  ADJUST FLOW VALVE":node==G_FLOWER?"A  TEND THE MOONFLOWER":"D-PAD WALK / B NOTES / START PAUSE");
  txt(c,2,26,36,busy?"PREPARING NEXT ROOM...":g->progress&G_ROUTE?"Rainwater is flowing to the garden.":"Field notes show the next task.");
 }
 if(g->notice)txt(c,3,3,33,"ROOM BUSY - TRY NOTEBOOK AGAIN");
 else if(save_status==14)txt(c,3,3,33,"UPDATING NOTEBOOK...");
 else if(save_status==1)txt(c,3,3,33,"NOTEBOOK SAVED");
 else if(save_status==4)txt(c,3,3,33,"NO NOTEBOOK YET - BEGIN A NIGHT");
 else if(save_status>=5 && save_status!=13)txt(c,3,3,33,"NOTEBOOK UNAVAILABLE - TRY LATER");
}

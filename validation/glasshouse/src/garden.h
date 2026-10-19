#ifndef GARDEN_H
#define GARDEN_H
#include "text_layout.h"
typedef unsigned short G16;
#define G_WALK_STEP 2
enum {G_FIELD,G_TALK,G_JOURNAL,G_PAUSE,G_TITLE,G_GUIDE,G_END};
enum {G_LEARNED=1,G_PUMP=2,G_ROUTE=4,G_BLOOM=8};
enum {G_NONE,G_CARETAKER,G_ENGINE,G_VALVE,G_FLOWER,G_DOOR};
enum {G_TRAVEL=1,G_SAVE=2,G_LEAVE=4,G_RESUME=8,G_EFFECT=16};
typedef struct {
 DLayout text;
 G16 area,x,y,progress,mode,menu,node,page,revealed,age,clock,tick,face,walking,choice,ready,request,sound,notice;
} Garden;
void garden_init(Garden *g);
void garden_enter(Garden *g,G16 area);
G16 garden_screen(Garden const *g);
G16 garden_near(Garden const *g);
G16 garden_open(Garden *g,G16 node);
G16 garden_step(Garden *g,G16 elapsed,G16 held,G16 hit);
int garden_position(G16 area,G16 x,G16 y);
int garden_save_valid(G16 area,G16 x,G16 flags);
void garden_draw(UiCanvas *c,Garden const *g,G16 hz,G16 busy,G16 save_status);
#endif

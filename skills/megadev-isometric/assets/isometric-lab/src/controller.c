#include "controller.h"
void iso_world_init(IsoWorld *w){
 for(IsoU16 i=0;i<sizeof(*w);i++)((volatile unsigned char *)w)[i]=0;
 iso_init(&w->game);
 for(IsoU16 i=0;i<ISO_STATIC;i++){w->items[i].box=iso_prop_box(i);w->items[i].id=i;}
 w->sort_status=iso_cache_init(&w->cache,w->items,ISO_STATIC);
}
IsoU16 iso_order(IsoWorld *w){
 IsoU16 links=0,count=iso_scene(&w->game,w->items,w->links,&links);
 w->sort_status=iso_sort_cached(&w->cache,&w->items[ISO_STATIC],1,w->links,links,w->order);
 return count;
}
IsoU16 iso_advance(IsoWorld *w,IsoU16 elapsed,IsoU16 held,IsoU16 pressed){
 w->events=0;
 if(pressed&128u){w->paused^=1;w->clock=0;return 0;}
 if(pressed&16u)w->cutaway^=1;
 if(pressed&32u)w->info^=1;
 if(w->paused)return 0;
 if(pressed&64u){w->tour=(w->tour+1u)&3u;iso_tour(&w->game,w->tour);w->events=1;}
 if(elapsed>20){elapsed=20;w->clamped++;}
 w->clock+=elapsed;IsoU16 steps=0;
 IsoU16 direction=((held&4u)?ISO_LEFT:0)|((held&8u)?ISO_RIGHT:0)|((held&1u)?ISO_UP:0)|((held&2u)?ISO_DOWN:0);
 while(w->clock>=5){w->clock-=5;iso_tick(&w->game,direction);steps++;}
 return steps;
}

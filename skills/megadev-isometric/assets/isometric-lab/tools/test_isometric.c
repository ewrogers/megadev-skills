#include "controller.h"
#define CHECK(c) do {if(!(c))return __LINE__;} while(0)
static int check_projection(void)
{
    IsoPoint p=iso_project(256,0,0);CHECK(p.x==16 && p.y==8);
    p=iso_project(0,256,0);CHECK(p.x==-16 && p.y==8);
    p=iso_project(256,256,512);CHECK(!p.x && p.y==-16);
    p=iso_project(-1,0,0);CHECK(p.x==-1 && p.y==-1); /* Floor, not truncation. */
    for(IsoS16 x=-4096;x<=4096;x+=256)for(IsoS16 y=-4096;y<=4096;y+=256) {
        p=iso_project(x,y,128);IsoPoint back;
        CHECK(iso_unproject(p,128,&back) && back.x==x && back.y==y);
    }
    IsoPoint out={123,456};CHECK(!iso_unproject((IsoPoint){1024,1024},0,&out) && out.x==123 && out.y==456);
    CHECK(!iso_unproject((IsoPoint){0,0},-1,&out));
    CHECK(!iso_unproject((IsoPoint){0,0},0,(IsoPoint *)0));
    return 0;
}
static int check_relations(void)
{
    IsoBox low={0,0,0,256,256,384},deck={0,0,448,256,256,512},high={0,0,512,256,256,896};
    CHECK(iso_relation(low,deck)==ISO_BEFORE && iso_relation(deck,low)==ISO_AFTER);
    CHECK(iso_relation(deck,high)==ISO_BEFORE && iso_relation(high,deck)==ISO_AFTER);
    CHECK(iso_relation(low,low)==ISO_AMBIGUOUS);
    CHECK(iso_relation(low,(IsoBox){1000,1000,0,1256,1256,384})==ISO_NONE);
    /* Rectangle overlap alone would invent a contradictory X/Y relationship. */
    CHECK(iso_relation((IsoBox){0,512,0,256,768,256},(IsoBox){512,0,0,768,256,256})==ISO_NONE);
    CHECK(iso_relation(low,(IsoBox){0,0,0,0,1,1})==ISO_INVALID);
    CHECK(iso_relation(low,(IsoBox){0,0,0,8193,1,1})==ISO_INVALID);
    return 0;
}
static int check_sort(void)
{
    IsoItem items[]={{{0,0,512,256,256,896},30},{{0,0,0,256,256,384},10},{{0,0,448,256,256,512},20}};
    IsoU16 order[ISO_MAX_ITEMS];CHECK(iso_sort(items,3,(IsoLink *)0,0,order)==ISO_SORT_OK);
    CHECK(order[0]==1 && order[1]==2 && order[2]==0);
    IsoItem swapped[]={items[2],items[0],items[1]};IsoU16 other[ISO_MAX_ITEMS];
    CHECK(iso_sort(swapped,3,(IsoLink *)0,0,other)==ISO_SORT_OK);
    for(IsoU16 i=0;i<3;i++)CHECK(items[order[i]].id==swapped[other[i]].id);
    IsoItem intersect[]={items[0],items[0]};intersect[1].id=40;
    CHECK(iso_sort(intersect,2,(IsoLink *)0,0,order)==ISO_SORT_AMBIGUOUS);
    IsoLink link={30,40};CHECK(iso_sort(intersect,2,&link,1,order)==ISO_SORT_OK && order[0]==0);
    IsoItem separate[]={{{0,0,0,16,16,16},3},{{1000,1000,0,1016,1016,16},1},{{2000,2000,0,2016,2016,16},2}};
    IsoLink cycle[]={{3,1},{1,2},{2,3}};
    CHECK(iso_sort(separate,3,cycle,3,order)==ISO_SORT_CYCLE);
    CHECK(order[0]!=order[1] && order[1]!=order[2] && order[0]!=order[2]);
    IsoLink bad={30,999};order[0]=999;
    CHECK(iso_sort(items,3,&bad,1,order)==ISO_SORT_INVALID && order[0]==999);
    CHECK(iso_sort(items,ISO_MAX_ITEMS+1,(IsoLink *)0,0,order)==ISO_SORT_INVALID);
    items[1].id=30;CHECK(iso_sort(items,3,(IsoLink *)0,0,order)==ISO_SORT_INVALID);
    IsoItem full[ISO_MAX_ITEMS];IsoLink chain[ISO_MAX_ITEMS];
    for(IsoU16 i=0;i<ISO_MAX_ITEMS;i++) {
        full[i]=(IsoItem){{(IsoS16)(i*256),0,0,(IsoS16)(i*256+16),16,16},(IsoU16)(100-i)};
        if(i)chain[i-1]=(IsoLink){full[i-1].id,full[i].id};
    }
    CHECK(iso_sort(full,ISO_MAX_ITEMS,chain,ISO_MAX_ITEMS-1,order)==ISO_SORT_OK);
    for(IsoU16 i=0;i<ISO_MAX_ITEMS;i++)CHECK(order[i]==i);
    chain[ISO_MAX_ITEMS-1]=(IsoLink){full[ISO_MAX_ITEMS-1].id,full[0].id};
    CHECK(iso_sort(full,ISO_MAX_ITEMS,chain,ISO_MAX_ITEMS,order)==ISO_SORT_CYCLE);
    return 0;
}
static int check_world(IsoU16 frames)
{
    IsoGame g;iso_init(&g);IsoItem items[ISO_MAX_ITEMS];IsoLink links[ISO_MAX_LINKS];IsoU16 nlinks,order[ISO_MAX_ITEMS];
    iso_scene(&g,items,links,&nlinks);IsoCache cache;CHECK(iso_cache_init(&cache,items,ISO_STATIC)==ISO_SORT_OK);
    for(IsoU16 stop=0;stop<4;stop++) {
        iso_tour(&g,stop);iso_scene(&g,items,links,&nlinks);
        CHECK(iso_sort(items,ISO_ITEMS,links,nlinks,order)==ISO_SORT_OK);
        IsoU16 cached[ISO_MAX_ITEMS];CHECK(iso_sort_cached(&cache,&items[ISO_STATIC],1,links,nlinks,cached)==ISO_SORT_OK);
        for(IsoU16 i=0;i<ISO_ITEMS;i++)CHECK(order[i]==cached[i]);
    }
    /* Walk onto the left stairs, across the deck, and down the right stairs. */
    g.x=480;g.y=1408;g.z=0;g.layer=0;
    IsoU16 heights=0;
    for(IsoU16 i=0;i<170;i++){iso_tick(&g,ISO_RIGHT|ISO_DOWN);if(g.z)heights++;}
    CHECK(heights>0 && g.x>2048 && !g.layer && !g.z && g.y==1408);
    /* Turning back while the footprint touches the first riser must not trap us. */
    g.x=500;g.y=1408;g.z=0;g.layer=0;
    iso_tick(&g,ISO_LEFT|ISO_UP);CHECK(g.x<500 && !g.layer);
    /* The lower avenue passes below the same deck without switching layers. */
    g.x=1280;g.y=1200;g.z=0;g.layer=0;
    for(IsoU16 i=0;i<30;i++)iso_tick(&g,ISO_DOWN);
    CHECK(!g.layer && !g.z && g.y>1536);
    /* An upper actor cannot leave a walkway edge into thin air. */
    iso_tour(&g,2);
    for(IsoU16 i=0;i<30;i++)iso_tick(&g,ISO_UP);
    CHECK(g.layer && g.y>=1320 && g.z==512);
    /* Ground footprint blocks a tower, independent of sprite pixels. */
    g.x=184;g.y=384;g.z=0;g.layer=0;
    for(IsoU16 i=0;i<60;i++)iso_tick(&g,ISO_RIGHT|ISO_DOWN);
    CHECK(g.x<=216 && g.y==384);
    /* Reuse static edges through a changing route; compare full and cached sort. */
    iso_init(&g);
    for(IsoU16 i=0;i<frames;i++) {
        if(!(i%100))iso_tour(&g,(i/100)%4);
        iso_tick(&g,(i%100)<50 ? ISO_RIGHT|ISO_DOWN : ISO_UP);
        iso_scene(&g,items,links,&nlinks);IsoU16 cached[ISO_MAX_ITEMS];
        IsoU16 full=iso_sort(items,ISO_ITEMS,links,nlinks,order);
        CHECK(full==ISO_SORT_OK);
        CHECK(iso_sort_cached(&cache,&items[ISO_STATIC],1,links,nlinks,cached)==full);
        for(IsoU16 j=0;j<ISO_ITEMS;j++)CHECK(order[j]==cached[j]);
    }
    /* Camera is derived after simulation and constrained to authored map space. */
    CHECK(g.camera_x>=64 && g.camera_x<=128 && g.camera_y>=0 && g.camera_y<=64);
    return 0;
}
static int check_cache_and_timing(void)
{
    IsoGame a,b;iso_init(&a);iso_init(&b);
    for(IsoU16 i=0;i<600;i++)iso_frame(&a,0,60,0);
    for(IsoU16 i=0;i<500;i++)iso_frame(&b,0,50,0);
    CHECK(a.ticks==600 && b.ticks==600 && a.phase==b.phase);
    IsoU16 tick=b.ticks,phase=b.phase;
    for(IsoU16 i=0;i<50;i++)CHECK(!iso_frame(&b,ISO_RIGHT,50,1));
    CHECK(b.ticks==tick && b.phase==phase && !iso_frame(&b,0,0,0));
    IsoItem items[ISO_MAX_ITEMS];IsoLink links[ISO_MAX_LINKS];IsoU16 count,order[ISO_MAX_ITEMS];IsoCache cache;
    iso_scene(&a,items,links,&count);CHECK(iso_cache_init(&cache,items,ISO_STATIC)==ISO_SORT_OK);
    items[0].id=999; /* The cache owns a copy, not a reference to temporary input. */
    CHECK(cache.fixed[0].id==0);
    IsoItem actor={0}; /* Initialized below; duplicate IDs must fail. */
    actor=cache.fixed[0];CHECK(iso_sort_cached(&cache,&actor,1,(IsoLink *)0,0,order)==ISO_SORT_INVALID);
    CHECK(iso_cache_init(&cache,(IsoItem *)0,1)==ISO_SORT_INVALID && !cache.valid);
    CHECK(iso_sort_cached(&cache,&actor,1,(IsoLink *)0,0,order)==ISO_SORT_INVALID);
    return 0;
}
static IsoWorld a,b;
int isometric_contract(void){
 int result;
 if((result=check_projection()))return result;
 if((result=check_relations()))return result;
 if((result=check_sort()))return result;
 if((result=check_world(40)))return result;
 if((result=check_cache_and_timing()))return result;
 iso_world_init(&a);iso_world_init(&b);
 for(IsoU16 i=0;i<60;i++)iso_advance(&a,5,8,0);
 for(IsoU16 i=0;i<50;i++)iso_advance(&b,6,8,0);
 CHECK(a.game.x==b.game.x && a.game.y==b.game.y && a.game.ticks==60 && b.game.ticks==60);
 iso_advance(&a,5,0,128);IsoU16 tick=a.game.ticks;
 iso_advance(&a,65535,8,64);CHECK(a.game.ticks==tick && a.paused);
 iso_advance(&a,5,0,128);CHECK(iso_advance(&a,65535,0,0)==4 && a.clamped==1);
 return 0;
}
#ifndef ISOMETRIC_TARGET_PROBE
int main(void){return isometric_contract();}
#endif

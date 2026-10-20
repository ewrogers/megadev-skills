#include "iso.h"
#define U(n) ((n)*ISO_UNIT)
/* Foot anchors are in map coordinates; z is 16 raw units per vertical pixel. */
const IsoProp iso_props[ISO_STATIC]={
    {896,1408,512,ISO_DECK},{1152,1408,512,ISO_DECK},{1408,1408,512,ISO_DECK},{1664,1408,512,ISO_DECK},
    {832,1312,0,ISO_POST},{1728,1312,0,ISO_POST},{832,1504,0,ISO_POST},{1728,1504,0,ISO_POST},
    {640,1408,0,ISO_RAMP_L},{1920,1408,0,ISO_RAMP_R},
    {384,384,0,ISO_TOWER},{2176,384,0,ISO_TOWER},
    {384,2048,0,ISO_TREE},{2176,2048,0,ISO_TREE},{1664,2176,0,ISO_FOUNTAIN}
};
IsoBox iso_prop_box(IsoU16 i)
{
    if(i>=ISO_STATIC)return (IsoBox){0};
    IsoProp p=iso_props[i];IsoS16 r=128,low=p.z,high=low+256;
    if(p.kind==ISO_DECK){low=448;high=512;}
    if(p.kind==ISO_POST){r=64;high=448;}
    if(p.kind==ISO_RAMP_L || p.kind==ISO_RAMP_R)high=512;
    if(p.kind==ISO_TOWER)high=768;
    if(p.kind==ISO_TREE)high=512;
    if(p.kind==ISO_FOUNTAIN){r=128;high=320;}
    return (IsoBox){p.x-r,p.y-r,low,p.x+r,p.y+r,high};
}
static IsoBool overlaps(IsoS16 x,IsoS16 y,IsoBox b)
{
    return x-40<b.x1 && x+40>b.x0 && y-40<b.y1 && y+40>b.y0;
}
static IsoBool try_move(IsoGame *g,IsoS16 x,IsoS16 y)
{
    if(x<64 || y<64 || x>U(10)-64 || y>U(10)-64)return 0;
    IsoU16 layer=g->layer;IsoBool lane=y>=U(5)+40 && y<=U(6)-40;
    if(layer) {
        if(!lane)return 0;
        if(x<U(2) || x>=U(8))layer=0;
    } else {
        IsoBool on_ramp=(x>=U(2)-40 && x<U(3)+40) || (x>=U(7)-40 && x<U(8)+40);
        if(y+40>U(5) && y-40<U(6) && on_ramp) {
            IsoBool left=g->x<U(2) && x<U(3) && ((x<U(2) && x<=g->x) || lane);
            IsoBool right=g->x>=U(8) && x>=U(7) && ((x>=U(8) && x>=g->x) || lane);
            if(left || right) {if(x>=U(2) && x<U(8))layer=1;}
            else return 0;
        }
    }
    for(IsoU16 i=0;i<ISO_STATIC;i++) {
        IsoU16 k=iso_props[i].kind;
        if(k==ISO_DECK || k==ISO_RAMP_L || k==ISO_RAMP_R)continue;
        IsoBox b=iso_prop_box(i);
        if(layer && b.z1<=512)continue;
        if(overlaps(x,y,b))return 0;
    }
    g->x=x;g->y=y;g->layer=layer;g->z=0;
    if(layer) {
        if(x<U(3))g->z=(IsoS16)(((x-U(2))/64+1)*128);
        else if(x>=U(7))g->z=(IsoS16)(((U(8)-1-x)/64+1)*128);
        else g->z=512;
    }
    return 1;
}
static void camera(IsoGame *g)
{
    IsoPoint p=iso_project(g->x,g->y,g->z);
    g->camera_x=256+p.x-160;g->camera_y=64+p.y-112;
    if(g->camera_x<64)g->camera_x=64;
    if(g->camera_x>128)g->camera_x=128;
    if(g->camera_y<0)g->camera_y=0;
    if(g->camera_y>32)g->camera_y=32;
}
void iso_tour(IsoGame *g,IsoU16 stop)
{
    static const IsoS16 points[4][3]={{384,1152,0},{640,1408,1},{1280,1408,1},{1280,1152,0}};
    stop%=4;g->x=points[stop][0];g->y=points[stop][1];g->layer=(IsoU16)points[stop][2];g->z=g->layer ? (stop==1 ? 384 : 512) : 0;
    g->walk=0;g->moving=0;camera(g);
}
void iso_init(IsoGame *g){*g=(IsoGame){0};iso_tour(g,0);}
void iso_tick(IsoGame *g,IsoU16 held)
{
    IsoS16 sx=((held&ISO_RIGHT)!=0)-((held&ISO_LEFT)!=0),sy=((held&ISO_DOWN)!=0)-((held&ISO_UP)!=0);
    IsoS16 dx=16*sx+16*sy,dy=-16*sx+16*sy;
    if(sx && sy){dx=dx*3/4;dy=dy*3/4;}
    IsoS16 oldx=g->x,oldy=g->y;
    if(dx)try_move(g,g->x+dx,g->y);
    if(dy)try_move(g,g->x,g->y+dy);
    g->moving=oldx!=g->x || oldy!=g->y;
    if(sx || sy)g->facing=sy<0 ? 0 : sy>0 ? 2 : sx>0 ? 1 : 3;
    if(g->moving)g->walk=(g->walk+1)%32;else g->walk=0;
    g->ticks++;camera(g);
}
IsoU16 iso_frame(IsoGame *g,IsoU16 held,IsoU16 hz,IsoBool paused)
{
    if(paused || (hz!=50 && hz!=60))return 0;
    g->phase+=60;IsoU16 n=0;while(g->phase>=hz){g->phase-=hz;iso_tick(g,held);n++;}return n;
}
IsoU16 iso_scene(const IsoGame *g,IsoItem *items,IsoLink *links,IsoU16 *link_count)
{
    for(IsoU16 i=0;i<ISO_STATIC;i++)items[i]=(IsoItem){iso_prop_box(i),i};
    items[ISO_STATIC]=(IsoItem){{g->x-40,g->y-40,g->z,g->x+40,g->y+40,g->z+384},ISO_STATIC};
    *link_count=0;
    /* The surface owner resolves sloped/stepped-volume proxy overlap, including
       the footprint margin at the two stair feet. This is not a generic tie. */
    for(IsoU16 ramp=8;ramp<=9;ramp++)if(overlaps(g->x,g->y,items[ramp].box))
        links[(*link_count)++]=(IsoLink){ramp,ISO_STATIC};
    return ISO_ITEMS;
}

#include "iso.h"
static IsoS32 floor_shift(IsoS32 n,IsoU16 bits){return n<0?-1-((-1-n)>>bits):n>>bits;}
IsoPoint iso_project(IsoS16 x,IsoS16 y,IsoS16 z)
{
    return (IsoPoint){(IsoS16)floor_shift((IsoS32)x-y,4),(IsoS16)floor_shift((IsoS32)x+y-2*(IsoS32)z,5)};
}
IsoBool iso_unproject(IsoPoint p,IsoS16 z,IsoPoint *out)
{
    if(!out || z<0 || z>2048 || p.x < -1024 || p.x>1024 || p.y < -1024 || p.y>1024)return 0;
    IsoS32 x=8*(IsoS32)p.x+16*(IsoS32)p.y+z,y=-8*(IsoS32)p.x+16*(IsoS32)p.y+z;
    if(x < -ISO_LIMIT || x>ISO_LIMIT || y < -ISO_LIMIT || y>ISO_LIMIT)return 0;
    *out=(IsoPoint){(IsoS16)x,(IsoS16)y};return 1;
}
static IsoBool valid(IsoBox b)
{
    return b.x0>=-ISO_LIMIT && b.y0>=-ISO_LIMIT && b.z0>=0 && b.x1<=ISO_LIMIT && b.y1<=ISO_LIMIT && b.z1<=2048 &&
        b.x0<b.x1 && b.y0<b.y1 && b.z0<b.z1;
}
static IsoBool interval(IsoS32 a0,IsoS32 a1,IsoS32 b0,IsoS32 b1){return a0<b1 && b0<a1;}
IsoU16 iso_relation(IsoBox a,IsoBox b)
{
    if(!valid(a) || !valid(b))return ISO_INVALID;
    /* Separating axes of the projected 3-D box hexagons: x-y, x-z, y-z. */
    if(!interval((IsoS32)a.x0-a.y1,(IsoS32)a.x1-a.y0,(IsoS32)b.x0-b.y1,(IsoS32)b.x1-b.y0) ||
       !interval((IsoS32)a.x0-a.z1,(IsoS32)a.x1-a.z0,(IsoS32)b.x0-b.z1,(IsoS32)b.x1-b.z0) ||
       !interval((IsoS32)a.y0-a.z1,(IsoS32)a.y1-a.z0,(IsoS32)b.y0-b.z1,(IsoS32)b.y1-b.z0))return ISO_NONE;
    IsoBool before=a.x1<=b.x0 || a.y1<=b.y0 || a.z1<=b.z0;
    IsoBool after=b.x1<=a.x0 || b.y1<=a.y0 || b.z1<=a.z0;
    if(before && !after)return ISO_BEFORE;
    if(after && !before)return ISO_AFTER;
    return ISO_AMBIGUOUS;
}
static const IsoItem *item_at(const IsoItem *fixed,IsoU16 n,const IsoItem *dynamic,IsoU16 i)
{
    return i<n ? &fixed[i] : &dynamic[i-n];
}
static IsoU16 sort(const IsoItem *fixed,IsoU16 fixed_count,const IsoU32 *fixed_edges,const IsoItem *dynamic,IsoU16 dynamic_count,
                const IsoLink *links,IsoU16 link_count,IsoU16 *order)
{
    IsoU16 count=fixed_count+dynamic_count;
    if(!count || count>ISO_MAX_ITEMS || link_count>ISO_MAX_LINKS || (dynamic_count && !dynamic) || !order || (link_count && !links))return ISO_SORT_INVALID;
    for(IsoU16 i=fixed_count;i<count;i++) {
        const IsoItem *item=item_at(fixed,fixed_count,dynamic,i);
        if(!valid(item->box))return ISO_SORT_INVALID;
        for(IsoU16 j=0;j<i;j++)if(item->id==item_at(fixed,fixed_count,dynamic,j)->id)return ISO_SORT_INVALID;
    }
    IsoU32 edges[ISO_MAX_ITEMS];IsoU16 indegree[ISO_MAX_ITEMS];
    for(IsoU16 i=0;i<ISO_MAX_ITEMS;i++){((volatile IsoU32 *)edges)[i]=0;((volatile IsoU16 *)indegree)[i]=0;}
    IsoU32 used=0;IsoU16 status=ISO_SORT_OK;
    for(IsoU16 i=0;i<fixed_count;i++)edges[i]=fixed_edges[i];
    for(IsoU16 i=0;i<link_count;i++) {
        IsoU16 a=count,b=count;
        for(IsoU16 j=0;j<count;j++) {
            IsoU16 id=item_at(fixed,fixed_count,dynamic,j)->id;
            if(id==links[i].before)a=j;
            if(id==links[i].after)b=j;
        }
        if(a==count || b==count || a==b)return ISO_SORT_INVALID;
        /* Explicit links replace inference for that pair, including cached edges. */
        edges[b]&=~((IsoU32)1<<a);edges[a]|=(IsoU32)1<<b;
        for(IsoU16 j=0;j<i;j++)if(links[j].before==links[i].after && links[j].after==links[i].before)return ISO_SORT_INVALID;
    }
    for(IsoU16 i=0;i<count;i++)for(IsoU16 j=i+1;j<count;j++) {
        if(i<fixed_count && j<fixed_count)continue;
        if((edges[i]&((IsoU32)1<<j)) || (edges[j]&((IsoU32)1<<i)))continue;
        IsoU16 relation=iso_relation(item_at(fixed,fixed_count,dynamic,i)->box,item_at(fixed,fixed_count,dynamic,j)->box);
        if(relation==ISO_BEFORE)edges[i]|=(IsoU32)1<<j;
        else if(relation==ISO_AFTER)edges[j]|=(IsoU32)1<<i;
        else if(relation==ISO_AMBIGUOUS)status=ISO_SORT_AMBIGUOUS;
    }
    for(IsoU16 i=0;i<count;i++)for(IsoU16 j=0;j<count;j++)if(edges[i]&((IsoU32)1<<j))indegree[j]++;
    for(IsoU16 step=0;step<count;step++) {
        IsoU16 pick=count;
        for(IsoU16 i=0;i<count;i++)if(!(used&((IsoU32)1<<i)) && !indegree[i] &&
            (pick==count || item_at(fixed,fixed_count,dynamic,i)->id<item_at(fixed,fixed_count,dynamic,pick)->id))pick=i;
        if(pick==count) {
            status=ISO_SORT_CYCLE;
            for(IsoU16 i=0;i<count;i++)if(!(used&((IsoU32)1<<i)) &&
                (pick==count || item_at(fixed,fixed_count,dynamic,i)->id<item_at(fixed,fixed_count,dynamic,pick)->id))pick=i;
        }
        order[step]=pick;used|=(IsoU32)1<<pick;
        for(IsoU16 j=0;j<count;j++)if((edges[pick]&((IsoU32)1<<j)) && indegree[j])indegree[j]--;
    }
    return status;
}
IsoU16 iso_sort(const IsoItem *items,IsoU16 count,const IsoLink *links,IsoU16 link_count,IsoU16 *order)
{
    return sort((const IsoItem *)0,0,(const IsoU32 *)0,items,count,links,link_count,order);
}
IsoU16 iso_cache_init(IsoCache *cache,const IsoItem *fixed,IsoU16 count)
{
    if(!cache)return ISO_SORT_INVALID;
    cache->valid=0;
    if(!fixed || !count || count>ISO_MAX_ITEMS)return ISO_SORT_INVALID;
    IsoU16 order[ISO_MAX_ITEMS];IsoU16 status=iso_sort(fixed,count,(const IsoLink *)0,0,order);
    if(status!=ISO_SORT_OK)return status;
    cache->count=count;
    for(IsoU16 i=0;i<count;i++) {
        cache->fixed[i]=fixed[i];cache->edges[i]=0;
        for(IsoU16 j=0;j<count;j++)if(i!=j && iso_relation(fixed[i].box,fixed[j].box)==ISO_BEFORE)cache->edges[i]|=(IsoU32)1<<j;
    }
    cache->valid=1;return ISO_SORT_OK;
}
IsoU16 iso_sort_cached(const IsoCache *cache,const IsoItem *dynamic,IsoU16 count,const IsoLink *links,IsoU16 link_count,IsoU16 *order)
{
    if(!cache || !cache->valid || cache->count>ISO_MAX_ITEMS || count>ISO_MAX_ITEMS-cache->count)return ISO_SORT_INVALID;
    return sort(cache->fixed,cache->count,cache->edges,dynamic,count,links,link_count,order);
}

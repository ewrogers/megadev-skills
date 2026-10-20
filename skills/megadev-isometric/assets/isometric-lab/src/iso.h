#ifndef ISOMETRIC_LAB_H
#define ISOMETRIC_LAB_H
typedef signed short IsoS16;typedef unsigned short IsoU16;
typedef signed int IsoS32;typedef unsigned int IsoU32;typedef unsigned char IsoBool;
#ifdef _WIN32
#define ISO_API __declspec(dllexport)
#else
#define ISO_API
#endif
#define ISO_UNIT 256
#define ISO_LIMIT 8192
#define ISO_MAX_ITEMS 24
#define ISO_MAX_LINKS 24
#define ISO_STATIC 15
#define ISO_ITEMS (ISO_STATIC+1)
enum {ISO_NONE,ISO_BEFORE,ISO_AFTER,ISO_AMBIGUOUS,ISO_INVALID};
enum {ISO_SORT_OK,ISO_SORT_AMBIGUOUS,ISO_SORT_CYCLE,ISO_SORT_INVALID};
enum {ISO_LEFT=1,ISO_RIGHT=2,ISO_UP=4,ISO_DOWN=8};
enum {ISO_DECK,ISO_POST,ISO_RAMP_L,ISO_RAMP_R,ISO_TOWER,ISO_TREE,ISO_FOUNTAIN,ISO_HERO};
typedef struct {IsoS16 x,y;} IsoPoint;
typedef struct {IsoS16 x0,y0,z0,x1,y1,z1;} IsoBox;
typedef struct {IsoBox box;IsoU16 id;} IsoItem;
typedef struct {IsoU16 before,after;} IsoLink;
typedef struct {IsoItem fixed[ISO_MAX_ITEMS];IsoU32 edges[ISO_MAX_ITEMS];IsoU16 count;IsoBool valid;} IsoCache;
typedef struct {IsoS16 x,y,z;IsoU16 kind;} IsoProp;
typedef struct {IsoS16 x,y,z,camera_x,camera_y;IsoU16 layer,facing,walk,phase,ticks;IsoBool moving;} IsoGame;
extern const IsoProp iso_props[ISO_STATIC];
ISO_API IsoPoint iso_project(IsoS16 x,IsoS16 y,IsoS16 z);
ISO_API IsoBool iso_unproject(IsoPoint screen,IsoS16 z,IsoPoint *world);
ISO_API IsoU16 iso_relation(IsoBox a,IsoBox b);
ISO_API IsoU16 iso_sort(const IsoItem *items,IsoU16 count,const IsoLink *links,IsoU16 link_count,IsoU16 *order);
ISO_API IsoU16 iso_cache_init(IsoCache *cache,const IsoItem *fixed,IsoU16 count);
ISO_API IsoU16 iso_sort_cached(const IsoCache *cache,const IsoItem *dynamic,IsoU16 count,const IsoLink *links,IsoU16 link_count,IsoU16 *order);
ISO_API IsoBox iso_prop_box(IsoU16 index);
ISO_API void iso_init(IsoGame *g);
ISO_API void iso_tour(IsoGame *g,IsoU16 stop);
ISO_API void iso_tick(IsoGame *g,IsoU16 held);
ISO_API IsoU16 iso_frame(IsoGame *g,IsoU16 held,IsoU16 hz,IsoBool paused);
ISO_API IsoU16 iso_scene(const IsoGame *g,IsoItem *items,IsoLink *support,IsoU16 *link_count);
#endif

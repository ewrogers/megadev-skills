#ifndef ISOMETRIC_CONTROLLER_H
#define ISOMETRIC_CONTROLLER_H
#include "iso.h"
typedef struct {
 IsoGame game;IsoCache cache;IsoItem items[ISO_MAX_ITEMS];IsoLink links[ISO_MAX_LINKS];
 IsoU16 order[ISO_MAX_ITEMS],sort_status,clock,paused,cutaway,info,tour,events,clamped;
} IsoWorld;
void iso_world_init(IsoWorld *w);
IsoU16 iso_advance(IsoWorld *w,IsoU16 elapsed300,IsoU16 held,IsoU16 pressed);
IsoU16 iso_order(IsoWorld *w);
#endif

#ifndef ISOMETRIC_PACKED_H
#define ISOMETRIC_PACKED_H
#include "iso.h"
/* Returns success only for an exact bounded decode; output may be partial on failure. */
ISO_API IsoU16 iso_unpack(unsigned char const *src,IsoU16 size,unsigned char *out,IsoU16 capacity);
#endif

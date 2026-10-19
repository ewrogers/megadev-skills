#include "levels.h"
/* Pointer-only seam executes the same production tile sampling code. */
LvU16 lp_word(LvArea const *a,LvU8 const *attrs,LvU16 plane,LvU16 x,LvU16 y,LvU16 debug){return lv_word(a,attrs,plane,x,y,debug);}

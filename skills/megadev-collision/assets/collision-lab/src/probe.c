#include "demo.h"
void cp_sweep(CcBox const *a,CcVec const *da,CcBox const *b,CcVec const *db,CcHit *out){*out=cc_sweep(*a,*da,*b,*db);}
void cp_ray(CcVec const *p,CcVec const *d,CcBox const *b,CcHit *out){*out=cc_ray(*p,*d,*b);}
void cp_slide(CcBox const *a,CcVec const *d,CcBox const *walls,CcU16 count,CcMove *out){*out=cc_slide(*a,*d,walls,count);}

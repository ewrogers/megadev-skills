#include "garden.h"
int garden_position(G16 area,G16 x,G16 y) {
 if(area>1 || x<16 || x>296 || y<48 || y>176)return 0;
 /* Same footprint as the two drawn central beds / workbenches. Player feet
    use a small 10x8 box; art can overlap the decorative rim above it. */
 if(x>102 && x<182 && ((y>64 && y<100) || (y>144 && y<176)))return 0;
 return 1;
}
int garden_save_valid(G16 area,G16 x,G16 flags) {
 if((area!=1 && area!=2) || flags&0xF000u)return 0;
 G16 progress=flags&15u,y=flags>>4;
 if((progress&G_ROUTE) && !(progress&G_PUMP))return 0;
 if((progress&G_BLOOM) && (progress&7u)!=7u)return 0;
 return garden_position((G16)(area-1u),x,y);
}

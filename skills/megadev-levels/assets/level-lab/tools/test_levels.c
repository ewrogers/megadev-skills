#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "levels.h"
#define CHECK(c) do {if(!(c)){fprintf(stderr,"Level contract failed at %d\n",__LINE__);return 1;}}while(0)
static unsigned char bytes[32768];
int main(int argc,char **argv) {
 CHECK(argc==2);FILE *f=fopen(argv[1],"rb");CHECK(f);size_t size=fread(bytes,1,sizeof bytes,f);CHECK(!ferror(f));fclose(f);
 LvWorld w;CHECK(lv_open(&w,bytes,(LvU32)size));CHECK(w.count==2 && w.areas[0].w==48 && w.areas[0].h==32);
 LvGame a,b;lv_init(&w,&a);lv_init(&w,&b);CHECK(a.x==80 && a.y==264 && !lv_blocked(&w.areas[0],80,264));
 CHECK(lv_solid(&w.areas[0],-1,0) && lv_solid(&w.areas[0],0,0));
 for(int i=0;i<60;i++)lv_advance(&w,&a,5,LV_RIGHT,0);
 for(int i=0;i<50;i++)lv_advance(&w,&b,6,LV_RIGHT,0);
 CHECK(!memcmp(&a,&b,sizeof a) && a.x==200 && a.camera_x==40 && a.tick==60);
 lv_advance(&w,&a,5,0,LV_PAUSE);b=a;
 for(int i=0;i<50;i++)lv_advance(&w,&a,6,LV_RIGHT,0);
 CHECK(!memcmp(&a,&b,sizeof a));
 lv_init(&w,&a);CHECK(lv_advance(&w,&a,65535,LV_RIGHT,0)==4 && a.dropped==1 && a.x==88);
 for(int i=0;i<4000;i++) {
  lv_advance(&w,&a,5,(i/180)%4==0?LV_RIGHT:(i/180)%4==1?LV_UP:(i/180)%4==2?LV_LEFT:LV_DOWN,0);
  CHECK(!lv_blocked(&w.areas[a.area],(LvS16)a.x,(LvS16)a.y));CHECK(a.camera_x<=448 && a.camera_y<=320);
 }
 for(int i=0;i<8;i++) {lv_advance(&w,&a,5,0,LV_TOUR);CHECK(!lv_blocked(&w.areas[a.area],(LvS16)a.x,(LvS16)a.y));}
 a.x=736;a.y=264;lv_advance(&w,&a,5,0,LV_USE);CHECK(a.area==1 && a.x==80 && a.visits==1);
 a.x=40;a.y=264;lv_advance(&w,&a,5,0,LV_USE);CHECK(a.area==0 && a.x==680 && a.visits==2);
 b=a;CHECK(!lv_spawn(&w,&a,2,0) && !memcmp(&a,&b,sizeof a));CHECK(!lv_spawn(&w,&a,0,8));
 LvPatch patch;unsigned char attrs[128]={0};CHECK(!lv_patch(&patch,&w.areas[0],attrs,0,0,16,0,0));CHECK(!patch.count && !patch.used);
 CHECK(lv_patch(&patch,&w.areas[0],attrs,0,0,8,8,0) && patch.count==4 && patch.used==192);
 LvWorld invalid;CHECK(!lv_open(&invalid,bytes,(LvU32)size-1) && !invalid.count);
 bytes[4]^=1;CHECK(!lv_open(&invalid,bytes,(LvU32)size));bytes[4]^=1;
 /* Directory offset corruption cannot make the parser read outside the buffer. */
 bytes[12]=255;CHECK(!lv_open(&invalid,bytes,(LvU32)size) && !invalid.count);
 puts("Level model, binary bounds, collision, cameras and connected exits passed");return 0;
}

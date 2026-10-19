#include "levels.h"
static LvU16 word(LvU8 const *p){return (LvU16)((p[0]<<8)|p[1]);}
static LvU32 longword(LvU8 const *p){return ((LvU32)word(p)<<16)|word(p+2);}
/* Validated dimension factors fit a word; no long-multiply runtime is assumed. */
static LvU32 product(LvU16 a,LvU16 b) {
#ifdef __m68k__
 LvU32 result=a;__asm__("mulu.w %1,%0":"+d"(result):"d"(b):"cc");return result;
#else
 return (LvU32)a*b;
#endif
}
static LvU16 cell(LvArea const *a,LvU16 x,LvU16 y){return (LvU16)(product(y,a->w)+x);}
static LvU16 cells(LvArea const *a){return cell(a,0,a->h);}
static LvU8 const *spawn_data(LvArea const *a){LvU16 n=cells(a);return a->data+20u+(LvU32)n*5u+(n&1u);}
static LvU8 const *exit_data(LvArea const *a){return spawn_data(a)+a->spawns*8u;}
LvU16 lv_tile(LvArea const *a,LvU16 plane,LvS16 x,LvS16 y) {
 if(plane>1 || x<0 || y<0 || x>=a->w || y>=a->h)return 0;
 return word(a->data+20u+((LvU32)plane*cells(a)+cell(a,(LvU16)x,(LvU16)y))*2u);
}
LvU16 lv_solid(LvArea const *a,LvS16 x,LvS16 y) {
 if(x<0 || y<0 || x>=a->w || y>=a->h)return 1;
 return a->data[20u+(LvU32)cells(a)*4u+cell(a,(LvU16)x,(LvU16)y)]!=0;
}
LvU16 lv_blocked(LvArea const *a,LvS16 x,LvS16 y) {
 if(x<5 || y<4 || x>(LvS16)(a->w*16u-5u) || y>(LvS16)(a->h*16u-4u))return 1;
 return lv_solid(a,(LvS16)((x-5)>>4),(LvS16)((y-4)>>4)) || lv_solid(a,(LvS16)((x+4)>>4),(LvS16)((y-4)>>4)) ||
 lv_solid(a,(LvS16)((x-5)>>4),(LvS16)((y+3)>>4)) || lv_solid(a,(LvS16)((x+4)>>4),(LvS16)((y+3)>>4));
}
LvU16 lv_open(LvWorld *world,LvU8 const *data,LvU32 bytes) {
 /* Fail closed: no partially installed world on malformed binary input. */
 world->count=0;
 if(!data || bytes<20 || data[0]!='W' || data[1]!='L' || data[2]!='D' || data[3]!='1' || word(data+4)!=1 || longword(data+8)!=bytes)return 0;
 LvU16 count=word(data+6);if(!count || count>LV_AREAS || bytes<12u+count*8u)return 0;
 LvU32 next=12u+count*8u,total=0;
 for(LvU16 i=0;i<count;i++) {
  LvU32 offset=longword(data+12u+i*8u),size=longword(data+16u+i*8u);
  if(offset!=next || size<20 || offset>bytes || size>bytes-offset)return 0;
  next=offset+size;LvU8 const *p=data+offset;LvArea *a=&world->areas[i];a->data=p;
  if(p[0]!='L' || p[1]!='V' || p[2]!='L' || p[3]!='1' || word(p+4)!=1)return 0;
  a->w=word(p+6);a->h=word(p+8);a->tiles=word(p+10);a->spawns=word(p+12);a->exits=word(p+14);
  if(a->w<20 || a->w>96 || a->h<12 || a->h>64 || !a->tiles || a->tiles>64 || !a->spawns || a->spawns>8 || a->exits>8)return 0;
  LvU32 n=product(a->w,a->h);total+=n;
  if(n>4096 || total>4096 || longword(p+16)!=n || size!=20u+n*5u+(n&1u)+a->spawns*8u+a->exits*16u)return 0;
  for(LvU16 j=0;j<(LvU16)(n*2u);j++)if((word(p+20u+j*2u)&0x3fffu)>a->tiles)return 0;
  for(LvU16 j=0;j<n;j++)if(p[20u+n*4u+j]>1)return 0;
  p=spawn_data(a);
  for(LvU16 j=0;j<a->spawns;j++,p+=8)if(word(p)!=j || word(p+6) || lv_blocked(a,(LvS16)word(p+2),(LvS16)word(p+4)))return 0;
 }
 if(next!=bytes)return 0;
 for(LvU16 i=0;i<count;i++) {
  LvArea const *a=&world->areas[i];LvU8 const *p=exit_data(a);
  for(LvU16 j=0;j<a->exits;j++,p+=16) {
   LvU16 x=word(p+2),y=word(p+4),w=word(p+6),h=word(p+8),area=word(p+10),spawn=word(p+12);
   if(word(p)!=j || word(p+14) || !w || !h || x>=a->w*16u || y>=a->h*16u || w>a->w*16u-x || h>a->h*16u-y || area>=count || spawn>=world->areas[area].spawns)return 0;
   for(LvU16 ty=y>>4;ty<=((y+h-1u)>>4);ty++)for(LvU16 tx=x>>4;tx<=((x+w-1u)>>4);tx++)if(lv_solid(a,(LvS16)tx,(LvS16)ty))return 0;
  }
 }
 world->count=count;return 1;
}
void lv_camera(LvWorld const *w,LvGame *s) {
 LvArea const *a=&w->areas[s->area];LvS16 x=(LvS16)s->x-160,y=(LvS16)s->y-96;
 LvU16 mx=(LvU16)(a->w*16u-320u),my=(LvU16)(a->h*16u-192u);
 s->camera_x=x<0?0:x>mx?mx:(LvU16)x;s->camera_y=y<0?0:y>my?my:(LvU16)y;
}
LvU16 lv_spawn(LvWorld const *w,LvGame *s,LvU16 area,LvU16 spawn) {
 if(area>=w->count || spawn>=w->areas[area].spawns)return 0;
 LvU8 const *p=spawn_data(&w->areas[area])+spawn*8u;
 s->area=area;s->x=word(p+2);s->y=word(p+4);s->moving=0;s->near=65535;lv_camera(w,s);return 1;
}
void lv_init(LvWorld const *w,LvGame *s) {
 s->area=s->x=s->y=s->camera_x=s->camera_y=s->face=s->moving=s->tick=s->paused=s->clock=s->pending=s->debug=s->waypoint=s->visits=s->events=s->dropped=0;
 s->near=65535;lv_spawn(w,s,0,0);
}
static void tick(LvWorld const *w,LvGame *s,LvU16 held,LvU16 hit) {
 s->tick++;s->moving=0;LvS16 dx=0,dy=0;
 if((held&12u)==LV_LEFT){dx=-2;s->face=2;}
 if((held&12u)==LV_RIGHT){dx=2;s->face=3;}
 if((held&3u)==LV_UP){dy=-2;s->face=1;}
 if((held&3u)==LV_DOWN){dy=2;s->face=0;}
 /* Two one-pixel substeps bound ordinary walking; fast shots use swept queries. */
 LvArea const *a=&w->areas[s->area];
 for(LvU16 i=0;i<2;i++) {
  if(dx && !lv_blocked(a,(LvS16)s->x+(dx>0?1:-1),(LvS16)s->y)){s->x=(LvU16)(s->x+(dx>0?1:-1));s->moving=1;}
  if(dy && !lv_blocked(a,(LvS16)s->x,(LvS16)s->y+(dy>0?1:-1))){s->y=(LvU16)(s->y+(dy>0?1:-1));s->moving=1;}
 }
 s->near=65535;LvU8 const *p=exit_data(a);
 for(LvU16 i=0;i<a->exits;i++,p+=16) {
  LvU16 x=word(p+2),y=word(p+4);
  if(s->x>=x && s->x-x<word(p+6) && s->y>=y && s->y-y<word(p+8)){s->near=i;break;}
 }
 if(hit&LV_USE) {
  if(s->near<a->exits){p=exit_data(a)+s->near*16u;lv_spawn(w,s,word(p+10),word(p+12));s->visits++;s->events|=LV_EVENT_TRAVEL;s->clock=0;return;}
  s->events|=LV_EVENT_NO;
 }
 lv_camera(w,s);
}
LvU16 lv_advance(LvWorld const *w,LvGame *s,LvU16 elapsed,LvU16 held,LvU16 pressed) {
 s->events=0;
 if(!w->count || s->area>=w->count)return 0;
 if(pressed&LV_PAUSE){s->paused^=1;s->clock=s->pending=0;return 0;}
 if(s->paused)return 0;
 if(pressed&LV_DEBUG){s->debug^=1;s->events|=LV_EVENT_WARP;}
 if(pressed&LV_TOUR){s->waypoint++;if(s->waypoint>=w->areas[s->area].spawns)s->waypoint=0;lv_spawn(w,s,s->area,s->waypoint);s->events|=LV_EVENT_WARP;}
 s->pending|=pressed&LV_USE;
 if(elapsed>20){elapsed=20;s->dropped++;}
 s->clock=(LvU16)(s->clock+elapsed);LvU16 steps=0;
 while(s->clock>=5){s->clock-=5;LvU16 hit=s->pending;s->pending=0;tick(w,s,held,hit);steps++;if(s->events&LV_EVENT_TRAVEL)break;}
 return steps;
}
LvU16 lv_word(LvArea const *a,LvU8 const *attrs,LvU16 plane,LvU16 x,LvU16 y,LvU16 debug) {
 LvU16 code=lv_tile(a,plane,(LvS16)(x>>1),(LvS16)(y>>1));
 if(debug){if(plane)return 0;code=lv_solid(a,(LvS16)(x>>1),(LvS16)(y>>1))?5:1;}
 if(!(code&0x3fff))return 0;
 LvU16 tile=(LvU16)((code&0x3fffu)-1u),qx=x&1u,qy=y&1u,flags=0;
 if(code&0x4000u){qx^=1;flags|=0x0800;}
 if(code&0x8000u){qy^=1;flags|=0x1000;}
 return (LvU16)(0x100u+tile*4u+qy*2u+qx)|flags|word(attrs+tile*2u);
}
LvU16 lv_patch(LvPatch *p,LvArea const *a,LvU8 const *attrs,LvU16 ox,LvU16 oy,LvU16 x,LvU16 y,LvU16 debug) {
 p->count=p->used=0;
 LvS16 dx=(LvS16)(x>>3)-(LvS16)(ox>>3),dy=(LvS16)(y>>3)-(LvS16)(oy>>3);
 if(dx<-1 || dx>1 || dy<-1 || dy>1)return 0;
 LvU16 left=x>>3,top=y>>3;
 for(LvU16 plane=0;plane<2;plane++) {
  LvU16 base=plane?0xC000:0xE000;
  if(dx) {
   LvU16 col=dx>0?(LvU16)(left+63u):left;LvRun *r=&p->runs[p->count++];
   r->address=(LvU16)(base+(col&63u)*2u);r->count=32;r->step=128;r->offset=p->used;
   for(LvU16 py=0;py<32;py++){LvU16 wy=(LvU16)(top+((py+32u-(top&31u))&31u));p->words[p->used++]=lv_word(a,attrs,plane,col,wy,debug);}
  }
  if(dy) {
   LvU16 row=dy>0?(LvU16)(top+31u):top;LvRun *r=&p->runs[p->count++];
   r->address=(LvU16)(base+(row&31u)*128u);r->count=64;r->step=2;r->offset=p->used;
   for(LvU16 px=0;px<64;px++){LvU16 wx=(LvU16)(left+((px+64u-(left&63u))&63u));p->words[p->used++]=lv_word(a,attrs,plane,wx,row,debug);}
  }
 }
 return 1;
}

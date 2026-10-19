#include "raster.h"
static const RtS wave[32]={0,1,2,3,4,4,5,5,5,5,5,4,4,3,2,1,0,-1,-2,-3,-4,-4,-5,-5,-5,-5,-5,-4,-4,-3,-2,-1};
void rt_init(RtWorld *w,RtU demo){w->x[0]=256;w->x[1]=384;w->y[0]=80;w->y[1]=160;w->line=144;w->tick=0;w->remainder=0;w->selected=0;w->enabled=1;w->waves=1;w->automatic=0;w->paused=0;w->demo=demo?1:0;w->direction=1;w->clamped=0;}
RtU rt_advance(RtWorld *w,RtU units,RtU held,RtU hit){
 if(hit&128u)w->paused^=1;
 if(w->paused)return 0;
 if(hit&64u){if(w->demo)w->selected^=1;else w->enabled^=1;}
 if(hit&16u){if(w->demo)w->enabled^=1;else w->waves^=1;}
 if(hit&32u)w->automatic^=1;
 if(units>20){units=20;if(w->clamped<65535)w->clamped++;}
 w->remainder+=units;RtU n=0;
 while(w->remainder>=5){
  w->remainder-=5;w->tick++;n++;RtU i=w->demo?w->selected:0;
  if((held&12)==8 && w->x[i]<480)w->x[i]+=2;
  if((held&12)==4 && w->x[i]>32)w->x[i]-=2;
  if(w->demo){
   if((held&3)==1 && w->y[i]>48)w->y[i]--;
   if((held&3)==2 && w->y[i]<192)w->y[i]++;
   if(w->automatic){RtU other=i^1u;w->x[other]=(RtU)(256+wave[(w->tick>>2)&31]*16);}
  }else{
   if((held&3)==1 && w->line>112)w->line--;
   if((held&3)==2 && w->line<176)w->line++;
   if(w->automatic && !(w->tick&3u)){
    if(w->line==176)w->direction=0;
    if(w->line==112)w->direction=1;
    if(w->direction)w->line++;else w->line--;
   }
  }
 }
 return n;
}
static RtU camera_x(RtU x){return x<160?0:x>352?192:(RtU)(x-160);}
static RtU camera_y(RtU y){return y<48?0:(RtU)(y-48);}
static void sprite(RtView *v,RtS x,RtS y,RtU size,RtU tile,RtU pal){
 RtU n=v->count;if(n==RT_SPRITES)return;
 if(n)v->sat[n-1][1]|=n;
 v->sat[n][0]=(RtU)(y+128);v->sat[n][1]=size;v->sat[n][2]=(RtU)(0x8000|tile|(pal<<13));v->sat[n][3]=(RtU)(x+128);v->count++;
}
RtU rt_plan_valid(RtPlan const *p){
 if(!p || p->line<64 || p->line>176 || p->flags>3 || p->enabled>1 || p->scroll_a>511 || p->scroll_b>511 || p->top_a>511 || p->top_b>511)return 0;
 if(p->wet0&~0xEEE || p->wet1&~0xEEE)return 0;
 return (p->flags&1)?p->line>=112 && p->table==0x8220:p->line==128 && p->table==0x8238;
}
void rt_build(RtWorld const *w,RtView *v){
 RtPlan *p=&v->plan;v->count=0;
 for(RtU k=0;k<4;k++)v->sat[0][k]=0;
 p->line=w->demo?128:w->line;p->flags=w->demo?0:(RtU)(1|(w->enabled?2:0));
 p->scroll_a=w->demo?(RtU)((camera_y(w->y[1])-136)&511):(RtU)((255-2*w->line)&255);
 p->scroll_b=w->demo?p->scroll_a:0;p->top_a=w->demo?(RtU)((camera_y(w->y[0])-32)&511):0;p->top_b=p->top_a;
 p->table=w->demo?0x8238:0x8220;p->wet0=0xE84;p->wet1=0xA42;p->enabled=w->demo?w->enabled:1;
 RtU cam0=camera_x(w->x[0]),cam1=camera_x(w->x[1]);
 for(RtU y=0;y<224;y++){
  if(w->demo){RtU cam=(!w->enabled || y<128)?cam0:cam1;v->scroll[y][0]=v->scroll[y][1]=(RtU)(0-cam)&1023;}
  else {RtU cam=cam0;RtS displacement=(w->waves && y>=w->line)?wave[((y-w->line)*2+(w->tick>>1))&31]:0;v->scroll[y][0]=(RtU)(displacement-(RtS)cam)&1023;v->scroll[y][1]=(RtU)(0-cam)&1023;}
 }
 /* Opaque 32px strips exactly fill H40's 320px row budget. Put them first:
    the hero intentionally disappears behind the surface/divider for 8 rows. */
 if(!w->demo || w->enabled)for(RtU i=0;i<10;i++)sprite(v,(RtS)(i<<5),(RtS)(w->demo?128:w->line-2),0xC00,(RtU)(0x88+(w->demo?16:((w->tick>>3)&3)*4)),0);
 if(w->demo){
  for(RtU i=0;i<2;i++){
   RtU camera=w->enabled?i:0;
   RtS y=(RtS)(w->y[i]-camera_y(w->y[camera])+((w->enabled && i)?136:32)-8);
   RtS x=(RtS)(w->x[i]-camera_x(w->x[camera])-8);
   if(x>=0 && x+16<=320 && y>=32 && y+16<=224)sprite(v,x,y,0x500,(RtU)(0x80+i*4),3);
  }
 }else {
  RtS x=(RtS)(w->x[0]-cam0-8),y=(RtS)(120+wave[(w->tick>>2)&31]/2);
  /* Split the actor into resident 8px cells at the full-width surface. Omit
     cells touching foam rather than relying on hardware fetch overflow. */
  for(RtU col=0;col<2;col++)for(RtU row=0;row<2;row++){
   RtS yy=(RtS)(y+(row<<3));
   if(yy+8<=(RtS)(w->line-2) || yy>=(RtS)(w->line+6))sprite(v,(RtS)(x+(col<<3)),yy,0,(RtU)(0x80+(col<<1)+row),3);
   /* Reflect only dry source cells. Sprites need their own projection: they
      do not inherit Plane A's vertical flip or line-scroll displacement. */
   RtS reflected=(RtS)(2*w->line-yy-8);
   if(yy+8<=(RtS)(w->line-2) && reflected>=(RtS)(w->line+6) && reflected+8<=224){
    RtS center=(RtS)(2*w->line-y-8);
    RtS dx=w->waves?wave[((center-w->line)*2+(w->tick>>1))&31]:0;
    sprite(v,(RtS)(x+(col<<3)+dx),reflected,0,(RtU)(0x1080+(col<<1)+row),3);
   }
  }
 }
}

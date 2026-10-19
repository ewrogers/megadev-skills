#include "fx.h"
/* These pools, limits and motion are Lab policy, not Megadev APIs. */
static FxU random16(FxWorld *w){FxU x=w->seed;x^=(FxU)(x<<7);x^=x>>9;x^=(FxU)(x<<8);return w->seed=x;}
FxU fx_alive(FxWorld const *w,FxU first,FxU end){FxU n=0;if(first>end || end>FX_POOL)return 0;for(FxU i=first;i<end;i++)if(w->p[i].life)n++;return n;}
void fx_init(FxWorld *w){for(FxU i=0;i<FX_POOL;i++){w->p[i].life=0;w->p[i].kind=0;}w->x=160;w->y=130;w->tick=0;w->remainder=0;w->seed=1979;w->weather=2;w->density=3;w->paused=0;w->shake=1;w->left=0;w->denied=0;w->clamped=0;}
static void emit(FxWorld *w,FxU first,FxU end,FxU kind,FxS x,FxS y,FxS vx,FxS vy,FxU life){
 for(FxU i=first;i<end;i++)if(!w->p[i].life){FxParticle *p=&w->p[i];p->x=x;p->y=y;p->vx=vx;p->vy=vy;p->life=life;p->kind=kind;return;}
 if(w->denied!=65535)w->denied++;
}
void fx_burst(FxWorld *w){
 static const FxS vx[12]={-40,-28,0,28,40,28,0,-28,-12,12,-20,20};
 static const FxS vy[12]={0,-28,-40,-28,0,28,40,28,-16,-20,-24,-28};
 FxU n=0;
 for(FxParticle *p=w->p;p<w->p+24 && n<12;p++)if(!p->life){
  p->x=(FxS)(w->x<<4);p->y=(FxS)(w->y<<4);p->vx=vx[n];p->vy=vy[n];p->life=n<8?32:52;p->kind=n<8?1:2;n++;
 }
 FxU refused=(FxU)(12-n);w->denied=w->denied>65535u-refused?65535u:(FxU)(w->denied+refused);
 if(w->shake)w->left=18;
}
static void step(FxWorld *w,FxU held){
 w->tick++;
 if((held&12)==8 && w->x<300)w->x+=2;
 if((held&12)==4 && w->x>20)w->x-=2;
 if((held&3)==1 && w->y>52)w->y-=2;
 if((held&3)==2 && w->y<204)w->y+=2;
 if(w->left)w->left--;
 for(FxU i=0;i<FX_POOL;i++){
  FxParticle *p=&w->p[i];if(!p->life)continue;
  p->life--;p->x+=p->vx;p->y+=p->vy;
  if(p->kind==1)p->vy+=2;
  if(p->kind==2 && !(w->tick&3u))p->vx=(FxS)(p->vx/2);
  if(p->kind==5 && !(w->tick&7u))p->vx=(FxS)((FxS)(random16(w)&15u)-7);
  if(p->x<-128 || p->x>5120 || p->y<512 || p->y>3584)p->life=0;
 }
 if(!(w->tick&3u))emit(w,24,32,3,(FxS)((w->x-4)<<4),(FxS)((w->y+5)<<4),-5,0,24);
 FxU target=w->weather?(FxU)((w->density+1)<<3):0;
 FxU current=fx_alive(w,32,64),added=0;
 for(FxU i=32;i<64;i++){
  FxParticle *p=&w->p[i];
  if(p->life && (w->weather==0 || p->kind!=(w->weather==1?4:5) || current>target)){p->life=0;current--;}
  if(!p->life && current<target && added<2){
   FxU kind=w->weather==1?4:5;
   p->y=p->kind==kind?512:(FxS)(512+(random16(w)&255u)*12u);p->kind=kind;p->x=(FxS)((random16(w)&511u)*10u);
   p->vx=w->weather==1?-8:(FxS)((FxS)(random16(w)&15u)-7);p->vy=w->weather==1?64:12;p->life=300;current++;added++;
  }
 }
}
FxU fx_advance(FxWorld *w,FxU units,FxU held,FxU hit){
 if(hit&128u){if(held&1u){w->shake^=1;if(!w->shake)w->left=0;}else w->paused^=1;}
 if(w->paused)return 0;
 if(hit&16u){w->weather++;if(w->weather==3)w->weather=0;}
 if(hit&32u)w->density=(w->density+1u)&3u;
 if(hit&64u)fx_burst(w);
 if(units>20){units=20;if(w->clamped!=65535)w->clamped++;}
 w->remainder+=units;FxU n=0;while(w->remainder>=5){w->remainder-=5;step(w,held);n++;}return n;
}
/* Fixed-size fast path. Charge every intersected 8-line band; the hero
   costs two width units and 8px effects cost one. */
void fx_build(FxWorld const *w,FxView *v){
 v->count=1;v->omitted=v->culled=0;v->shake_x=v->shake_y=0;
 for(FxU b=0;b<28;b++)v->bands[b]=0;
 if(w->left && w->shake){FxS amount=w->left>8?2:1;v->shake_x=(w->left&1u)?amount:-amount;v->shake_y=(w->left&2u)?amount:-amount;}
 FxS hx=(FxS)(w->x-8)+v->shake_x,hy=(FxS)(w->y-8)+v->shake_y;
 v->sat[0][0]=(FxU)(hy+128);v->sat[0][1]=0x500;v->sat[0][2]=0xE080;v->sat[0][3]=(FxU)(hx+128);
 FxU first=(FxU)hy>>3,last=(FxU)(hy+15)>>3;
 for(FxU b=first;b<=last;b++)v->bands[b]=2;
 FxU n=1;
 for(FxParticle const *p=w->p;p<w->p+FX_POOL;p++){
  if(!p->life)continue;
  /* Decorations use whole-cell culling; reject negative Q12.4 values before
     logical shifts, avoiding signed division in this hot path. */
  if(p->x<0 || p->x>=5120 || p->y<512 || p->y>=3584){v->culled++;continue;}
  FxS x=(FxS)((FxU)p->x>>4)+v->shake_x,y=(FxS)((FxU)p->y>>4)+v->shake_y;
  if(x<0 || x>=320 || y<32 || y>=224){v->culled++;continue;}
  FxU top=(FxU)y>>3,bottom=(FxU)(y+7)>>3;if(bottom>27)bottom=27;
  if(n==FX_SAT || v->bands[top]>=16 || v->bands[bottom]>=16){v->omitted++;continue;}
  v->bands[top]++;if(bottom!=top)v->bands[bottom]++;
  FxU tile=0x84;
  if(p->kind==1)tile+=(p->life>20?0:p->life>10?1:2);
  else if(p->kind==2)tile+=3+(p->life<24);
  else if(p->kind==3)tile+=5+(p->life<12);
  else if(p->kind==4)tile+=7;
  else tile+=8;
  v->sat[n-1][1]|=n;
  v->sat[n][0]=(FxU)(y+128);v->sat[n][1]=0;v->sat[n][2]=(FxU)(0xE000|tile);v->sat[n][3]=(FxU)(x+128);n++;
 }
 v->count=n;
}

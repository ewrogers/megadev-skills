#include "enemy.h"
#define CHECK(x) do {if(!(x))return __LINE__;}while(0)
static AiWorld a,b;
static AiSearch q;
static AiMap map;
static AiU8 route[AI_CELLS];
int enemy_contract(void){
 CHECK(ai_product(65535u,65535u)==4294836225u);
 CHECK(ai_product(0,65535u)==0 && ai_product(319,192)==61248u);
 for(AiU16 i=0;i<AI_CELLS;i++)((volatile AiU8 *)map.solid)[i]=0;
 map.revision=1;q.status=AI_IDLE;
 CHECK(ai_begin(&q,&map,0,239,2,7));
 CHECK(!ai_begin(&q,&map,1,2,0,1));
 CHECK(ai_work(&q,&map,0)==0 && q.expanded==0);
 while(q.status==AI_WORK)CHECK(ai_work(&q,&map,65535)<=AI_BUDGET);
 CHECK(q.status==AI_FOUND && q.expanded<=AI_CELLS && q.tail<=AI_CELLS);
 CHECK(ai_path(&q,route,AI_CELLS)==30);
 route[0]=77;CHECK(ai_path(&q,route,1)==0 && route[0]==77);
 CHECK(ai_result_valid(&q,&map,2,7));CHECK(!ai_result_valid(&q,&map,1,7));CHECK(!ai_result_valid(&q,&map,2,8));
 q.status=AI_IDLE;CHECK(ai_begin(&q,&map,0,239,0,1));map.revision++;
 CHECK(!ai_work(&q,&map,12) && q.status==AI_CANCELLED);
 q.status=AI_IDLE;CHECK(!ai_begin(&q,&map,240,1,0,1));
 for(AiU16 y=0;y<AI_H;y++)map.solid[y*AI_W+10]=1;
 CHECK(ai_begin(&q,&map,0,239,0,1));
 while(q.status==AI_WORK)ai_work(&q,&map,12);
 CHECK(q.status==AI_NO_PATH);
 for(AiU16 i=0;i<AI_CELLS;i++)((volatile AiU8 *)map.solid)[i]=0;
 map.solid[1]=map.solid[AI_W]=1;
 CHECK(!ai_ray(&map,8,8,24,24));CHECK(!ai_ray(&map,-1,8,24,24));
 map.solid[1]=map.solid[AI_W]=0;CHECK(ai_ray(&map,8,8,312,184));
 static AiEnemy e;e.state=AI_PATROL;e.x=e.y=40;e.face=3;
 CHECK(ai_sees(&map,&e,80,40));CHECK(!ai_sees(&map,&e,8,40));
 map.solid[2*AI_W+3]=1;CHECK(!ai_sees(&map,&e,80,40));
 ai_init(&a);ai_init(&b);
 for(AiU16 i=0;i<60;i++)ai_advance(&a,5,AI_RIGHT,0);
 for(AiU16 i=0;i<50;i++)ai_advance(&b,6,AI_RIGHT,0);
 for(AiU16 i=0;i<sizeof(a);i++)CHECK(((AiU8 *)&a)[i]==((AiU8 *)&b)[i]);
 CHECK(a.tick==60 && !ai_blocked(&a.map,a.x,a.y));
 ai_init(&a);CHECK(ai_advance(&a,65535,0,AI_STUN_BUTTON)==4);
 CHECK(a.clamped==1 && a.cooldown==177 && a.work<=AI_BUDGET*4u);
 ai_advance(&a,5,0,AI_PAUSE);AiU16 tick=a.tick,cooldown=a.cooldown;
 for(AiU16 i=0;i<50;i++)ai_advance(&a,6,AI_RIGHT,AI_STUN_BUTTON);
 CHECK(a.tick==tick && a.cooldown==cooldown);
 ai_init(&a);a.x=88;a.y=64;a.enemies[0].face=0;
 ai_advance(&a,5,0,0);CHECK(a.enemies[0].state==AI_ALERT && (a.events&AI_EVENT_ALERT));
 for(AiU16 i=0;i<25;i++)ai_advance(&a,5,0,0);
 CHECK(a.enemies[0].state==AI_CHASE);
 a.x=a.enemies[0].x;a.y=a.enemies[0].y;
 ai_advance(&a,5,0,AI_STUN_BUTTON);CHECK(a.enemies[0].state==AI_STUNNED);
 CHECK(a.hp==3 && (a.events&AI_EVENT_STUN));
 AiU16 generation=a.enemies[0].generation;
 ai_advance(&a,5,0,AI_STUN_BUTTON);CHECK(a.enemies[0].generation==generation);
 /* Recycling/stunning an actor invalidates an outstanding route. */
 a.job.status=AI_IDLE;CHECK(ai_begin(&a.job,&a.map,21,218,0,generation));
 ai_change_state(&a.enemies[0],AI_STUNNED,180);ai_advance(&a,5,0,0);
 CHECK(a.discarded==1 && a.job.status!=AI_WORK);
 /* A patrol left behind must search and return rather than chase forever. */
 ai_init(&a);a.enemies[0].state=AI_CHASE;a.enemies[0].memory=1;a.enemies[0].visible=0;
 ai_advance(&a,5,0,0);CHECK(a.enemies[0].state==AI_SEARCH);
 for(AiU16 i=0;i<91;i++)ai_advance(&a,5,0,0);
 CHECK(a.enemies[0].state==AI_RETURN || a.enemies[0].state==AI_PATROL);
 /* An obstructed return really schedules a search and follows its detour. */
 ai_init(&a);a.x=296;a.y=168;
 a.enemies[0].x=136;a.enemies[0].y=40;
 ai_change_state(&a.enemies[0],AI_RETURN,0);
 a.enemies[0].target_x=88;a.enemies[0].target_y=40;
 for(AiU16 i=0;i<500;i++)ai_advance(&a,5,0,0);
 CHECK(a.completed>0 && !ai_blocked(&a.map,a.enemies[0].x,a.enemies[0].y));
 CHECK(a.enemies[0].x>=87 && a.enemies[0].x<=89 && a.enemies[0].state==AI_PATROL);
 /* The sentry faces its target; a wall blocks fire even with stale visibility. */
 ai_init(&a);a.x=264;a.y=88;a.enemies[2].state=AI_CHASE;
 a.enemies[2].memory=180;a.enemies[2].visible=1;
 ai_advance(&a,5,0,0);CHECK((a.events&AI_EVENT_FIRE) && a.enemies[2].face==0);
 a.enemies[2].cooldown=0;a.map.solid[3*AI_W+16]=1;
 ai_advance(&a,5,0,0);CHECK(!(a.events&AI_EVENT_FIRE));
 ai_init(&a);a.x=a.enemies[0].x;a.y=a.enemies[0].y;
 ai_advance(&a,5,0,0);CHECK(a.hp==2 && (a.events&AI_EVENT_HURT));
 ai_advance(&a,5,0,0);CHECK(a.hp==2);
 /* Long deterministic play keeps all actors and work inside their bounds. */
 ai_init(&a);
 for(AiU16 i=0;i<2400;i++){
  AiU16 held=(i&128u)?AI_RIGHT:AI_LEFT;
  ai_advance(&a,5,held,(i%180u)==0?AI_STUN_BUTTON:0);
  CHECK(a.work<=AI_BUDGET && !ai_blocked(&a.map,a.x,a.y));
  for(AiU16 n=0;n<AI_ENEMIES;n++)CHECK(!ai_blocked(&a.map,a.enemies[n].x,a.enemies[n].y));
 }
 return 0;
}
#ifndef ENEMY_TARGET_PROBE
int main(void){return enemy_contract();}
#endif

#ifndef ENEMY_LAB_H
#define ENEMY_LAB_H
/* Application-owned AI, not Megadev APIs. Native pixels and a 60 Hz model. */
typedef unsigned char AiU8;
typedef unsigned short AiU16;
typedef signed short AiS16;
typedef unsigned int AiU32;
#define AI_W 20
#define AI_H 12
#define AI_CELLS 240
#define AI_NONE 255u
#define AI_ENEMIES 3
#define AI_BOLTS 6
#define AI_PATH 64
#define AI_BUDGET 12u
#define AI_UP 1u
#define AI_DOWN 2u
#define AI_LEFT 4u
#define AI_RIGHT 8u
#define AI_RESET 16u
#define AI_VIEW 32u
#define AI_STUN_BUTTON 64u
#define AI_PAUSE 128u
enum {AI_IDLE,AI_WORK,AI_FOUND,AI_NO_PATH,AI_INVALID,AI_CANCELLED};
enum {AI_PATROL,AI_ALERT,AI_CHASE,AI_SEARCH,AI_RETURN,AI_STUNNED};
enum {AI_EVENT_ALERT=1,AI_EVENT_FIRE=2,AI_EVENT_HURT=4,AI_EVENT_STUN=8,AI_EVENT_LENS=16,AI_EVENT_CLEAR=32};
typedef struct {AiU8 solid[AI_CELLS];AiU16 revision;} AiMap;
typedef struct {
 AiU8 parent[AI_CELLS],queue[AI_CELLS];
 AiU16 head,tail,status,start,goal,owner,generation,revision,expanded,last_work;
} AiSearch;
typedef struct {
 AiS16 x,y,home_x,home_y,target_x,target_y;
 AiU16 kind,state,face,timer,memory,cooldown,generation,replan,patrol,visible;
 AiU8 route[AI_PATH],length,cursor;
} AiEnemy;
typedef struct {AiS16 x,y,dx,dy;AiU16 life;} AiBolt;
typedef struct {
 AiMap map;AiSearch job;AiEnemy enemies[AI_ENEMIES];AiBolt bolts[AI_BOLTS];
 AiS16 x,y;
 AiU16 face,moving,tick,hp,hurt,cooldown,pulse,lenses,complete,game_over;
 AiU16 paused,clock,pending,view,events,sense_cursor,job_cursor;
 AiU16 work,work_high,completed,no_path,discarded,dropped,clamped;
} AiWorld;
extern char const ai_rows[AI_H][AI_W+1];
#if defined(_WIN32)
#define AI_API __declspec(dllexport)
#else
#define AI_API
#endif
AI_API AiU32 ai_product(AiU16 a,AiU16 b);
AI_API AiU16 ai_begin(AiSearch *q,AiMap const *map,AiU16 start,AiU16 goal,AiU16 owner,AiU16 generation);
AI_API AiU16 ai_work(AiSearch *q,AiMap const *map,AiU16 budget);
AI_API AiU16 ai_path(AiSearch const *q,AiU8 *out,AiU16 capacity);
AI_API AiU16 ai_result_valid(AiSearch const *q,AiMap const *map,AiU16 owner,AiU16 generation);
AI_API AiU16 ai_ray(AiMap const *map,AiS16 x0,AiS16 y0,AiS16 x1,AiS16 y1);
AI_API AiU16 ai_sees(AiMap const *map,AiEnemy const *e,AiS16 x,AiS16 y);
AI_API AiU16 ai_blocked(AiMap const *map,AiS16 x,AiS16 y);
AI_API void ai_init(AiWorld *s);
AI_API void ai_change_state(AiEnemy *e,AiU16 state,AiU16 timer);
AI_API AiU16 ai_advance(AiWorld *s,AiU16 elapsed300,AiU16 held,AiU16 pressed);
#endif

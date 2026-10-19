#ifndef ADVENTURE_GAMEPLAY_H
#define ADVENTURE_GAMEPLAY_H
/* Game-owned model, not Megadev SDK functions. Positions use quarter pixels. */
typedef unsigned short GpU16;
typedef signed short GpS16;
#define GP_ENEMIES 4
#define GP_ITEMS 6
#define GP_EFFECTS 12
#define GP_UP 1
#define GP_DOWN 2
#define GP_LEFT 4
#define GP_RIGHT 8
#define GP_PULSE 64
#define GP_RESET 16
#define GP_PAUSE 128
#define GP_EVENT_GEM 1
#define GP_EVENT_HURT 2
#define GP_EVENT_PULSE 4
#define GP_EVENT_AREA 8
#define GP_EVENT_CLEAR 16
#define GP_EVENT_RESPAWN 32
typedef struct {GpS16 x,y; GpU16 age,kind;} GpEffect;
typedef struct {GpS16 x,y; GpU16 direction,stun;} GpEnemy;
typedef struct {
 GpS16 x,y;
 GpEnemy enemies[GP_ENEMIES];
 GpEffect effects[GP_EFFECTS];
 GpU16 area,gems[2],hp,hurt,cooldown,pulse,face,moving,tick;
 GpU16 camera_x,camera_y,paused,clock,pending,events,dropped,rejected,visits,complete;
} Gameplay;
void gp_init(Gameplay *s);
GpU16 gp_advance(Gameplay *s,GpU16 elapsed300,GpU16 held,GpU16 pressed);
GpU16 gp_solid(GpU16 area,GpS16 tx,GpS16 ty);
GpU16 gp_blocked(GpU16 area,GpS16 x,GpS16 y);
void gp_move(Gameplay *s,GpS16 dx,GpS16 dy);
GpU16 gp_spawn(Gameplay *s,GpS16 x,GpS16 y,GpU16 kind);
GpS16 gp_item_x(GpU16 area,GpU16 i);
GpS16 gp_item_y(GpU16 area,GpU16 i);
void gp_camera(Gameplay *s);
#endif

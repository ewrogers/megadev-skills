#ifndef COOP_H
#define COOP_H
#include "input.h"
#ifdef _WIN32
#define MP_API __declspec(dllexport)
#else
#define MP_API
#endif
typedef unsigned short MpU16;
typedef signed short MpS16;
#define MP_PLAYERS 4
#define MP_VIEW_W 320
#define MP_VIEW_H 176
#define MP_WORLD_W 512
#define MP_WORLD_H 256
#define MP_NONE 8
#define MP_JOIN 1
#define MP_PULSE 2
#define MP_HURT 4
#define MP_PICKUP 8
#define MP_RETURN 16
#define MP_CLEAR 32
typedef struct {
  MpS16 x, y;
  MpU16 socket, kind, active, connected, hp, hurt, respawn, cooldown, pulse,
      face, walk, moving, grace, leave;
} MpPlayer;
typedef struct {
  MpS16 x, y;
  MpU16 stun, direction;
} MpEnemy;
typedef struct {
  MpPlayer players[4];
  MpEnemy enemies[3];
  MpU16 ticks, remainder, paused, started, demo, events, coins, complete,
      clamped;
  MpS16 camera_x, camera_y;
} MpGame;
MP_API void mp_init(MpGame *g);
MP_API void mp_advance(MpGame *g, MpU16 elapsed, InSample const samples[8],
                       MpU16 const pressed[8]);
MP_API void mp_demo(MpGame *g);
MP_API void mp_restart(MpGame *g);
MP_API void mp_camera(MpGame *g);
MP_API int mp_solid(MpS16 x, MpS16 y);
MP_API int mp_valid(MpS16 x, MpS16 y);
MP_API void mp_hit(MpGame *g, MpU16 player);
MP_API MpS16 mp_coin_x(MpU16 i);
MP_API MpS16 mp_coin_y(MpU16 i);
#endif

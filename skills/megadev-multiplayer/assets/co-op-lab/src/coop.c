#include "coop.h"
static MpS16 abs16(MpS16 n) { return n < 0 ? -n : n; }
static MpU16 pad(InSample const *s) {
  return s->status == IN_OK && (s->kind == IN_PAD3 || s->kind == IN_PAD6);
}
int mp_solid(MpS16 x, MpS16 y) {
  if (x < 1 || y < 1 || x >= 31 || y >= 15)
    return 1;
  if ((x == 9 || x == 22) && ((y >= 3 && y <= 6) || (y >= 11 && y <= 12)))
    return 1;
  if (x >= 14 && x <= 17 && y >= 8 && y <= 10)
    return 1;
  return 0;
}
int mp_valid(MpS16 x, MpS16 y) {
  if (x < 24 || y < 24 || x > MP_WORLD_W * 4 - 24 || y > MP_WORLD_H * 4 - 24)
    return 0;
  return !mp_solid((x - 20) >> 6, (y - 20) >> 6) &&
         !mp_solid((x + 19) >> 6, (y - 20) >> 6) &&
         !mp_solid((x - 20) >> 6, (y + 19) >> 6) &&
         !mp_solid((x + 19) >> 6, (y + 19) >> 6);
}
MpS16 mp_coin_x(MpU16 i) {
  static MpS16 const v[8] = {80, 176, 336, 432, 432, 336, 176, 80};
  return i < 8 ? v[i] * 4 : 0;
}
MpS16 mp_coin_y(MpU16 i) {
  static MpS16 const v[8] = {64, 48, 48, 64, 208, 224, 224, 208};
  return i < 8 ? v[i] * 4 : 0;
}
static void clear_player(MpPlayer *p) {
  p->x = p->y = 0;
  p->socket = MP_NONE;
  p->kind = IN_NONE;
  p->active = p->connected = p->hp = p->hurt = p->respawn = p->cooldown =
      p->pulse = p->face = p->walk = p->moving = p->grace = p->leave = 0;
}
void mp_init(MpGame *g) {
  for (MpU16 i = 0; i < 4; i++)
    clear_player(&g->players[i]);
  for (MpU16 i = 0; i < 3; i++) {
    g->enemies[i].x = (MpS16)(224 + i * 144);
    g->enemies[i].y = (MpS16)(448 + i * 176);
    g->enemies[i].stun = g->enemies[i].direction = 0;
  }
  g->ticks = g->remainder = g->paused = g->started = g->demo = g->events =
      g->coins = g->complete = g->clamped = 0;
  g->demo_stage = g->demo_hold = 0;
  g->camera_x = 0;
  g->camera_y = 32;
}
static int safe(MpGame const *g, MpS16 x, MpS16 y) {
  if (!mp_valid(x, y))
    return 0;
  for (MpU16 i = 0; i < 3; i++)
    if (!g->enemies[i].stun && abs16(x - g->enemies[i].x) < 112 &&
        abs16(y - g->enemies[i].y) < 112)
      return 0;
  return 1;
}
/* At most 25 candidates; chosen living teammate, then existing shelter.
   Players intentionally do not collide with one another. */
static void spawn(MpGame *g, MpU16 who) {
  MpS16 cx = 256, cy = 640;
  for (MpU16 i = 0; i < 4; i++)
    if (i != who && g->players[i].active && g->players[i].hp &&
        g->players[i].connected) {
      cx = g->players[i].x;
      cy = g->players[i].y;
      break;
    }
  MpPlayer *p = &g->players[who];
  MpU16 found = 0;
  for (MpU16 i = 0; i < 25; i++) {
    MpS16 x = (MpS16)(cx + ((MpS16)(i % 5) - 2) * 48),
          y = (MpS16)(cy + ((MpS16)(i / 5) - 2) * 48);
    MpU16 fits = 1;
    for (MpU16 j = 0; j < 4; j++)
      if (j != who && g->players[j].active && g->players[j].hp &&
          g->players[j].connected) {
        if (abs16(x - g->players[j].x) > 1088 ||
            abs16(y - g->players[j].y) > 512)
          fits = 0;
      }
    if (fits && safe(g, x, y)) {
      p->x = x;
      p->y = y;
      found = 1;
      break;
    }
  }
  if (!found) {
    p->x = cx;
    p->y = cy;
  }
  p->hp = 3;
  p->hurt = 90;
  p->respawn = 0;
  p->cooldown = 0;
  p->pulse = 0;
  p->moving = 0;
}
void mp_camera(MpGame *g) {
  MpS16 minx = 2048, miny = 1024, maxx = 0, maxy = 0;
  MpU16 count = 0;
  for (MpU16 i = 0; i < 4; i++) {
    MpPlayer const *p = &g->players[i];
    if (!p->active || !p->hp || !p->connected)
      continue;
    if (p->x < minx)
      minx = p->x;
    if (p->x > maxx)
      maxx = p->x;
    if (p->y < miny)
      miny = p->y;
    if (p->y > maxy)
      maxy = p->y;
    count++;
  }
  if (!count)
    return;
  MpS16 x = (MpS16)((minx + maxx) / 8 - 160),
        y = (MpS16)((miny + maxy) / 8 - 88);
  g->camera_x = x < 0 ? 0 : x > 192 ? 192 : x;
  g->camera_y = y < 0 ? 0 : y > 80 ? 80 : y;
}
static int move_ok(MpGame const *g, MpU16 who, MpS16 x, MpS16 y) {
  if (!mp_valid(x, y))
    return 0;
  for (MpU16 i = 0; i < 4; i++) {
    MpPlayer const *p = &g->players[i];
    if (i == who || !p->active || !p->connected || !p->hp)
      continue;
    if (abs16(x - p->x) > 1088 || abs16(y - p->y) > 512)
      return 0;
  }
  return 1;
}
void mp_hit(MpGame *g, MpU16 who) {
  if (who >= 4)
    return;
  MpPlayer *p = &g->players[who];
  if (!p->active || !p->connected || !p->hp || p->hurt || g->complete)
    return;
  p->hp--;
  p->hurt = 75;
  g->events |= MP_HURT;
  if (!p->hp) {
    p->respawn = 180;
    p->pulse = 0;
  }
}
static void step(MpGame *g, MpU16 const held[4], MpU16 const hits[4]) {
  g->ticks++;
  for (MpU16 i = 0; i < 3; i++) {
    MpEnemy *e = &g->enemies[i];
    if (e->stun) {
      e->stun--;
      continue;
    }
    MpS16 x = (MpS16)(e->x + (e->direction ? -3 : 3));
    if (!mp_valid(x, e->y) || x < 160 || x > 1880)
      e->direction ^= 1;
    else
      e->x = x;
  }
  for (MpU16 i = 0; i < 4; i++) {
    MpPlayer *p = &g->players[i];
    if (!p->active || !p->connected)
      continue;
    if (p->cooldown)
      p->cooldown--;
    if (p->pulse)
      p->pulse--;
    if (p->hurt)
      p->hurt--;
    if (p->respawn) {
      if (!--p->respawn) {
        spawn(g, i);
        g->events |= MP_RETURN;
      }
      continue;
    }
    if (!p->hp)
      continue;
    MpS16 dx = 0, dy = 0;
    MpU16 h = held[i];
    if ((h & 12) == 4) {
      dx = -6;
      p->face = 2;
    }
    if ((h & 12) == 8) {
      dx = 6;
      p->face = 3;
    }
    if ((h & 3) == 1) {
      dy = -6;
      p->face = 1;
    }
    if ((h & 3) == 2) {
      dy = 6;
      p->face = 0;
    }
    if (dx && dy) {
      dx = dx < 0 ? -4 : 4;
      dy = dy < 0 ? -4 : 4;
    }
    p->moving = 0;
    if (dx && move_ok(g, i, p->x + dx, p->y)) {
      p->x += dx;
      p->moving = 1;
    }
    if (dy && move_ok(g, i, p->x, p->y + dy)) {
      p->y += dy;
      p->moving = 1;
    }
    if (p->moving)
      p->walk = (p->walk + 1) & 31u;
    if ((hits[i] & IN_A) && !p->cooldown) {
      p->pulse = 20;
      p->cooldown = 180;
      g->events |= MP_PULSE;
      for (MpU16 j = 0; j < 3; j++)
        if (abs16(p->x - g->enemies[j].x) < 192 &&
            abs16(p->y - g->enemies[j].y) < 192)
          g->enemies[j].stun = 150;
    }
    for (MpU16 j = 0; j < 3; j++)
      if (!g->enemies[j].stun && abs16(p->x - g->enemies[j].x) < 40 &&
          abs16(p->y - g->enemies[j].y) < 40)
        mp_hit(g, i);
    for (MpU16 j = 0; j < 8; j++)
      if (!(g->coins & (1u << j)) && abs16(p->x - mp_coin_x(j)) < 44 &&
          abs16(p->y - mp_coin_y(j)) < 44) {
        g->coins |= 1u << j;
        g->events |= MP_PICKUP;
      }
  }
  if (g->coins == 255 && !g->complete) {
    g->complete = 1;
    for (MpU16 i = 0; i < 4; i++)
      g->players[i].moving = 0;
    g->events |= MP_CLEAR;
  }
  mp_camera(g);
}
void mp_demo(MpGame *g) {
  mp_init(g);
  g->started = 1;
  g->demo = 1;
  for (MpU16 i = 0; i < 4; i++) {
    MpPlayer *p = &g->players[i];
    p->active = p->connected = 1;
    p->socket = MP_NONE;
    p->kind = IN_PAD3;
    spawn(g, i);
  }
  mp_camera(g);
}
void mp_restart(MpGame *g) {
  if (g->demo) {
    mp_demo(g);
    return;
  }
  MpPlayer saved[4];
  for (MpU16 i = 0; i < 4; i++)
    saved[i] = g->players[i];
  mp_init(g);
  for (MpU16 i = 0; i < 4; i++)
    if (saved[i].active) {
      g->players[i] = saved[i];
      g->players[i].x = 256;
      g->players[i].y = 640;
      spawn(g, i);
      g->started = 1;
    }
  mp_camera(g);
}
void mp_advance(MpGame *g, MpU16 elapsed, InSample const samples[8],
                MpU16 const pressed[8]) {
  MpU16 held[4] = {0, 0, 0, 0}, hits[4] = {0, 0, 0, 0}, pause_request = 0;
  g->events = 0;
  if (elapsed > 20) {
    elapsed = 20;
    g->clamped++;
  }
  if (!g->demo) {
    for (MpU16 i = 0; i < 4; i++) {
      MpPlayer *p = &g->players[i];
      if (!p->active)
        continue;
      InSample const *s = &samples[p->socket];
      if (!pad(s) || s->kind != p->kind) {
        p->connected = 0;
        p->moving = 0;
        p->grace += elapsed;
        if (p->grace >= 1500 || (pad(s) && s->kind != p->kind))
          clear_player(p);
        continue;
      }
      if (!p->connected) {
        MpU16 hp = p->hp, respawn = p->respawn, cool = p->cooldown;
        p->connected = 1;
        spawn(g, i);
        p->hp = hp;
        p->respawn = respawn;
        p->cooldown = cool;
      }
      p->grace = 0;
      held[i] = s->held;
      hits[i] = pressed[p->socket];
      if ((s->held & (IN_START | IN_B)) == (IN_START | IN_B)) {
        p->leave += elapsed;
        held[i] = hits[i] = 0;
        if (p->leave >= 300)
          clear_player(p);
      } else {
        p->leave = 0;
        if (hits[i] & IN_START)
          pause_request = 1;
      }
    }
    if (pause_request)
      g->paused ^= 1;
    for (MpU16 socket = 0; socket < 8; socket++)
      if (pad(&samples[socket]) && (pressed[socket] & IN_START)) {
        MpU16 claimed = 0;
        for (MpU16 j = 0; j < 4; j++)
          if (g->players[j].active && g->players[j].socket == socket)
            claimed = 1;
        if (claimed)
          continue;
        for (MpU16 i = 0; i < 4; i++)
          if (!g->players[i].active) {
            MpPlayer *p = &g->players[i];
            clear_player(p);
            p->active = p->connected = 1;
            p->socket = socket;
            p->kind = samples[socket].kind;
            spawn(g, i);
            g->events |= MP_JOIN;
            g->started = 1;
            break;
          }
      }
  } else {
    /* Illustrative authored tour, not a new pathfinding service. Four bots
       follow a collision-safe circuit, light every lantern and repeat. */
    static MpS16 const route[15][2] = {
        {80, 64},   {112, 32},  {176, 32},  {176, 48},  {336, 48},
        {336, 32},  {432, 32},  {432, 64},  {432, 208}, {432, 224},
        {336, 224}, {176, 224}, {112, 224}, {80, 224},  {80, 208}};
    MpU16 arrived = 1;
    for (MpU16 i = 0; i < 4; i++) {
      MpS16 tx = route[g->demo_stage][0] * 4 + (i & 1 ? 24 : -24);
      MpS16 ty = route[g->demo_stage][1] * 4 + (i & 2 ? 24 : -24);
      MpPlayer const *p = &g->players[i];
      if (p->respawn)
        continue;
      MpS16 dx = tx - p->x, dy = ty - p->y;
      if (abs16(dx) > 6)
        held[i] |= dx < 0 ? IN_LEFT : IN_RIGHT;
      if (abs16(dy) > 6)
        held[i] |= dy < 0 ? IN_UP : IN_DOWN;
      if (abs16(dx) > 30 || abs16(dy) > 30)
        arrived = 0;
      if (!p->cooldown)
        hits[i] = IN_A;
    }
    if (arrived && g->demo_stage < 14)
      g->demo_stage++;
    if (g->complete) {
      g->demo_hold += elapsed;
      if (g->demo_hold >= 1200) {
        mp_demo(g);
        return;
      }
    }
  }
  MpU16 any = 0;
  for (MpU16 i = 0; i < 4; i++)
    if (g->players[i].active)
      any++;
  if (!any) {
    g->started = 0;
    g->paused = 0;
    g->remainder = 0;
    return;
  }
  mp_camera(g);
  if (g->paused) {
    g->remainder = 0;
    return;
  }
  g->remainder += elapsed;
  while (g->remainder >= 5) {
    g->remainder -= 5;
    if (g->complete)
      g->ticks++;
    else
      step(g, held, hits);
    for (MpU16 i = 0; i < 4; i++)
      hits[i] = 0;
  }
}

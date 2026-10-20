#include "coop.h"
#define CHECK(test)                                                            \
  do {                                                                         \
    if (!(test))                                                               \
      return __LINE__;                                                         \
  } while (0)
static InSample samples[8];
static MpU16 hits[8];
static void empty(void) {
  for (MpU16 i = 0; i < 8; i++) {
    in_clear(&samples[i], IN_ABSENT);
    hits[i] = 0;
  }
}
static void pad_on(MpU16 s, MpU16 held) {
  samples[s].kind = IN_PAD3;
  samples[s].status = IN_OK;
  samples[s].held = held;
}
static void join(MpGame *g, MpU16 s) {
  pad_on(s, 0);
  hits[s] = IN_START;
  mp_advance(g, 5, samples, hits);
  hits[s] = 0;
}
int coop_contract(void) {
  MpGame g;
  empty();
  mp_init(&g);
  CHECK(!g.started);
  join(&g, 4);
  CHECK(g.players[0].socket == 4 && g.players[0].active && !g.paused);
  join(&g, 0);
  join(&g, 3);
  join(&g, 7);
  join(&g, 2);
  CHECK(g.players[1].socket == 0 && g.players[2].socket == 3 &&
        g.players[3].socket == 7);
  hits[4] = IN_START;
  hits[0] = IN_START;
  mp_advance(&g, 5, samples, hits);
  CHECK(g.paused);
  hits[4] = hits[0] = 0;
  MpU16 t = g.ticks;
  mp_advance(&g, 20, samples, hits);
  CHECK(g.ticks == t);
  hits[4] = IN_START;
  mp_advance(&g, 5, samples, hits);
  hits[4] = 0;
  CHECK(!g.paused);
  g.players[1].hurt = 0;
  mp_hit(&g, 1);
  CHECK(g.players[1].hp == 2);
  mp_hit(&g, 1);
  CHECK(g.players[1].hp == 2);
  g.players[1].hurt = 0;
  mp_hit(&g, 1);
  g.players[1].hurt = 0;
  mp_hit(&g, 1);
  CHECK(g.players[1].respawn == 180 && !g.players[1].hp);
  MpS16 other = g.players[0].x;
  g.players[0].cooldown = 120;
  for (MpU16 i = 0; i < 179; i++)
    mp_advance(&g, 5, samples, hits);
  CHECK(g.players[1].respawn == 1 && g.players[0].x == other &&
        !g.players[0].cooldown);
  mp_advance(&g, 5, samples, hits);
  CHECK(g.players[1].hp == 3 && !g.players[1].respawn &&
        g.players[1].hurt == 90);
  CHECK(mp_valid(g.players[1].x, g.players[1].y));
  // Disconnect releases control; claimed slot survives errors, but does not
  // heal.
  g.players[1].hp = 2;
  in_clear(&samples[0], IN_TIMEOUT);
  mp_advance(&g, 5, samples, hits);
  CHECK(!g.players[1].connected && g.players[1].active);
  pad_on(0, 0);
  mp_advance(&g, 5, samples, hits);
  CHECK(g.players[1].connected && g.players[1].hp == 2);
  in_clear(&samples[3], IN_ABSENT);
  for (MpU16 i = 0; i < 300; i++)
    mp_advance(&g, 5, samples, hits);
  CHECK(!g.players[2].active && g.players[3].socket == 7);
  // Vacant player numbers can be reused; existing players never compact.
  hits[2] = IN_START;
  mp_advance(&g, 5, samples, hits);
  hits[2] = 0;
  CHECK(g.players[2].socket == 2 && g.players[3].socket == 7);
  // Explicit leave is bounded and doesn't pause or rejoin on its final tick.
  pad_on(2, IN_START | IN_B);
  for (MpU16 i = 0; i < 60; i++)
    mp_advance(&g, 5, samples, hits);
  CHECK(!g.players[2].active && !g.paused);
  // Not every input device is a gamepad.
  samples[3].kind = IN_MOUSE;
  samples[3].status = IN_OK;
  hits[3] = IN_START;
  mp_advance(&g, 5, samples, hits);
  CHECK(!g.players[2].active);
  hits[3] = 0;
  // All actors may disappear together; respawn still runs without camera reset.
  for (MpU16 i = 0; i < 4; i++)
    if (g.players[i].active) {
      g.players[i].hp = 0;
      g.players[i].respawn = 1;
    }
  mp_advance(&g, 5, samples, hits);
  CHECK(g.players[0].hp == 3 && g.players[1].hp == 3);
  // PAL/NTSC clocks use 300Hz units; movement and cooldowns agree at equal
  // time.
  MpGame a, b;
  empty();
  mp_init(&a);
  join(&a, 0);
  for (unsigned n = 0; n < sizeof(a); n++)
    ((volatile unsigned char *)&b)[n] = ((unsigned char *)&a)[n];
  pad_on(0, IN_RIGHT);
  for (MpU16 i = 0; i < 60; i++)
    mp_advance(&a, 5, samples, hits);
  for (MpU16 i = 0; i < 50; i++)
    mp_advance(&b, 6, samples, hits);
  CHECK(a.ticks == b.ticks && a.players[0].x == b.players[0].x &&
        a.players[0].hurt == b.players[0].hurt);
  // Camera: map bounds and all joined living feet remain inside safe margins.
  mp_demo(&g);
  empty();
  MpU16 saw_clear = 0, saw_restart = 0;
  for (MpU16 n = 0; n < 2400; n++) {
    if (g.complete)
      saw_clear = 1;
    else if (saw_clear)
      saw_restart = 1;
    mp_advance(&g, 5, samples, hits);
    CHECK(g.camera_x >= 0 && g.camera_x <= 192 && g.camera_y >= 0 &&
          g.camera_y <= 80);
    for (MpU16 i = 0; i < 4; i++) {
      MpPlayer *p = &g.players[i];
      CHECK(mp_valid(p->x, p->y));
      if (p->hp) {
        CHECK(p->x / 4 - g.camera_x >= 12 && p->x / 4 - g.camera_x < 308);
        CHECK(p->y / 4 - g.camera_y >= 20 && p->y / 4 - g.camera_y < 164);
      }
    }
  }
  CHECK(saw_clear && saw_restart);
  for (MpU16 i = 0; i < 300; i++)
    mp_advance(&g, 5, samples, hits);
  CHECK(g.demo);
  // Held A doesn't re-trigger: only edges are admitted; overload is capped.
  mp_init(&g);
  empty();
  join(&g, 0);
  pad_on(0, IN_A);
  hits[0] = IN_A;
  mp_advance(&g, 5, samples, hits);
  CHECK(g.players[0].cooldown == 180);
  hits[0] = 0;
  for (MpU16 i = 0; i < 190; i++)
    mp_advance(&g, 5, samples, hits);
  CHECK(!g.players[0].cooldown && !g.players[0].pulse);
  t = g.ticks;
  mp_advance(&g, 300, samples, hits);
  CHECK((MpU16)(g.ticks - t) == 4 && g.clamped == 1);
  // Clearing stops gameplay but keeps the celebration clock alive.
  MpS16 x = g.players[0].x, y = g.players[0].y;
  g.coins = 255;
  mp_advance(&g, 5, samples, hits);
  CHECK(g.complete && (g.events & MP_CLEAR));
  t = g.ticks;
  mp_advance(&g, 5, samples, hits);
  CHECK(g.ticks == (MpU16)(t + 1) && !(g.events & MP_CLEAR));
  CHECK(g.players[0].x == x && g.players[0].y == y);
  mp_restart(&g);
  CHECK(g.players[0].active && g.players[0].socket == 0 && !g.complete);
  mp_demo(&g);
  g.complete = 1;
  mp_restart(&g);
  CHECK(g.demo && !g.complete && g.players[0].socket == MP_NONE);
  mp_advance(&g, 5, samples, hits);
  CHECK(g.demo && g.started);
  return 0;
}
#ifndef COOP_TARGET
#include <stdio.h>
int main(void) {
  int n = coop_contract();
  if (n)
    fprintf(stderr, "Co-op contract failed at line %d\n", n);
  return n ? 1 : 0;
}
#endif

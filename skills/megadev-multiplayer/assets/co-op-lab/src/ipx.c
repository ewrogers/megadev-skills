#include "art_meta.h"
#include "coop.h"
#include "gfx.h"
#include "lab.h"
#include "platform.h"
#include "shared.h"
#include <main/bios.h>
#include <main/vdp.h>
#include <system.h>
volatile u32 main_magic = 0x12345678, main_zero[8];
volatile u16 main_ticks, room_runs;
MpGame game;
extern u8 const res_font[], res_actors[], res_ground[], res_palettes[];
extern void lab_sound(u16 tick, u16 event, u16 paused);
extern int coop_contract(void);
#define ACTOR 0x80u
#define GROUND 0x100u
#define SPR_CAP 48u
#define HUD 240u
_Static_assert(ACTOR + ART_COUNT <= GROUND &&
                   (GROUND + 16u) * 32u < BIOS_VDP_DEFAULT_WINDOW,
               "VRAM overlap");
typedef struct {
  GfxList sprites;
  u16 hud[HUD], x, y;
} Frame;
_Static_assert(sizeof(Frame) == 1804, "Frame ABI changed");
static Frame frames[2];
static InSample samples[8];
static InSlot inputs[8];
static u16 stage, hz, title_drawn = 2, tour_result;
static volatile u16 published, video_fault, submitted, upload_high;
static void reg(u16 i, u16 v) {
  u16 w = 0x8000u | (i << 8) | v;
  bios_vdp_regs[i] = w;
  vdp_ctrl_16 = w;
}
void lab_line(char const *s, u16 x, u16 y) {
  char line[41];
  u16 n = 0;
  if (x >= 40 || y >= 28)
    return;
  while (s[n] && n < 40 - x) {
    line[n] = s[n];
    n++;
  }
  line[n] = (char)255;
  lab_print(line,
            to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA + y * 128u + x * 2u) | VRAM_W);
}
void lab_hex(u16 v, u16 x, u16 y) {
  char s[5];
  for (u16 i = 0; i < 4; i++)
    s[i] = "0123456789ABCDEF"[(v >> (12 - i * 4)) & 15];
  s[4] = 0;
  lab_line(s, x, y);
}
void lab_screen(char const *s) { lab_line(s, 1, 8); }
void vblank_user(void) {
  main_ticks++;
  u16 index = published;
  if (!index)
    return;
  if (index > 2) {
    video_fault = 1;
    published = 0;
    return;
  }
  Frame const *f = &frames[index - 1];
  u16 count = f->sprites.count;
  if (count > SPR_CAP) {
    video_fault = 1;
    published = 0;
    return;
  }
  u16 v = (u16)(*(volatile u16 *)0xc00008u >> 8);
  if (!(vdp_ctrl_16 & VDPSTAT_MASK_VBLANK_IN_PROGRESS) || v < 224 || v >= 232)
    return;
  vdp_ctrl_32 = to_vdp_addr(BIOS_VDP_DEFAULT_SPRLIST) | VRAM_W;
  for (u16 i = 0; i < (count ? count : 1); i++)
    for (u16 w = 0; w < 4; w++)
      vdp_data_16 = f->sprites.sat[i][w];
  for (u16 y = 0; y < 6; y++) {
    vdp_ctrl_32 = to_vdp_addr(BIOS_VDP_DEFAULT_WINDOW + y * 128u) | VRAM_W;
    for (u16 x = 0; x < 40; x++)
      vdp_data_16 = f->hud[y * 40u + x];
  }
  vdp_ctrl_32 = to_vdp_addr(BIOS_VDP_DEFAULT_HSCROLL) | VRAM_W;
  vdp_data_16 = 0;
  vdp_data_16 = (u16)-f->x;
  vdp_ctrl_32 = VSRAM_W;
  vdp_data_16 = 0;
  vdp_data_16 = (u16)(f->y - 48u);
  u16 bytes = (count ? count : 1) * 8u + HUD * 2u + 8u;
  if (bytes > upload_high)
    upload_high = bytes;
  submitted++;
  published = 0;
}
static void text(Frame *f, u16 x, u16 y, u16 p, char const *s) {
  while (*s && x < 40 && y < 6)
    f->hud[y * 40u + x++] = (u16)(0x8000u | (p << 13) | (u8)*s++);
}
static u16 seconds(u16 units, u16 rate) {
  u16 n = 0;
  while (units) {
    n++;
    units = units > rate ? units - rate : 0;
  }
  return n;
}
static void number(Frame *f, u16 x, u16 y, u16 n) {
  char s[2] = {(char)('0' + n), 0};
  text(f, x, y, 0, s);
}
static void tile(Frame *f, u16 x, u16 y, u16 t, u16 pal) {
  if (x < 40 && y < 6)
    f->hud[y * 40u + x] = (u16)(0x8000u | (pal << 13) | GROUND | t);
}
static void sprite(Frame *f, s16 x, s16 y, u16 w, u16 h, u16 t, u16 p,
                   u16 flip) {
  if (y < 48 || y >= 224)
    return;
  gfx_sprite(&f->sprites, x, y, w, h, (u16)(ACTOR + t), p, 1, flip);
}
void coop_publish(void) {
  if (published)
    return;
  Frame *f = &frames[stage];
  gfx_begin(&f->sprites, SPR_CAP);
  f->x = game.camera_x;
  f->y = game.camera_y;
  for (u16 i = 0; i < HUD; i++)
    f->hud[i] = 0x8020;
  if (!game.started) {
    text(f, 1, 0, 0, "THE LANTERN KEEPERS");
    text(f, 34, 0, 0, hz == 50 ? "PAL" : "NTSC");
    text(f, 1, 2, 0, "CO-OP LAB / UP TO FOUR PLAYERS");
    text(f, 1, 4, 0, "EACH PAD: PRESS START TO JOIN");
  } else
    for (u16 i = 0; i < 4; i++) {
      MpPlayer const *p = &game.players[i];
      u16 x = (i & 1u) * 20u + 1u, y = (i >> 1) * 3u, pal = i == 3 ? 0 : i + 1;
      text(f, x, y, pal, "P");
      number(f, x + 1, y, i + 1);
      if (!p->active) {
        text(f, x + 4, y, 0, "START TO JOIN");
        continue;
      }
      for (u16 hp = 0; hp < 3; hp++)
        tile(f, x + 4 + hp * 2u, y, p->hp > hp ? 11 : 12, pal);
      if (!p->connected) {
        text(f, x, y + 1, 0, "PAD LOST");
        number(f, x + 10, y + 1, seconds((u16)(1500u - p->grace), 300));
      } else if (p->respawn) {
        text(f, x, y + 1, 0, "RETURN IN");
        number(f, x + 10, y + 1, seconds(p->respawn, 60));
      } else {
        text(f, x, y + 1, 0, "A");
        for (u16 n = 0; n < 8; n++)
          tile(f, x + 3 + n, y + 1, p->cooldown <= n * 23u ? 10 : 12, 0);
      }
      if (game.demo)
        text(f, x, y + 2, 0, "CPU TOUR");
      else {
        text(f, x, y + 2, 0, "PAD");
        number(f, x + 4, y + 2, (p->socket >> 2) + 1);
        char s[2] = {(char)('A' + (p->socket & 3)), 0};
        text(f, x + 5, y + 2, 0, s);
      }
    }
  if (game.paused)
    text(f, 13, 2, 0, "PAUSED / START");
  if (game.complete) {
    for (u16 x = 0; x < 40; x++)
      f->hud[80 + x] = 0x8020;
    text(f, 7, 2, 0, "GARDEN ALIGHT!  A PLAY AGAIN");
  }
  if (video_fault || tour_result)
    text(f, 1, 5, 0, "CHECK FAILED");
  if (game.started) {
    /* Important actors first; ambient decorations never remove gameplay. */
    for (u16 i = 0; i < 4; i++) {
      MpPlayer const *p = &game.players[i];
      if (!p->active || !p->connected || !p->hp)
        continue;
      s16 x = p->x / 4 - game.camera_x - 8, y = p->y / 4 - game.camera_y + 28;
      u16 pal = i == 3 ? 0 : i + 1;
      if (!p->hurt || (game.ticks & 4u))
        sprite(f, x, y, 16, 24,
               (u16)(p->face * 12u + (p->moving ? (p->walk >> 4) * 6u : 0)),
               pal, 0);
      if (p->pulse)
        sprite(f, x - 8, y - 4, 32, 32,
               (u16)(ART_PULSE + (p->pulse < 10 ? 16 : 0)), pal, 0);
    }
    for (u16 i = 0; i < 3; i++) {
      MpEnemy const *e = &game.enemies[i];
      s16 x = e->x / 4 - game.camera_x - 8, y = e->y / 4 - game.camera_y + 36;
      sprite(f, x, y, 16, 16, ART_MOTH + ((game.ticks >> 4) & 1u) * 4u, 3,
             e->direction);
      if (e->stun)
        sprite(f, x + 4, y - 8, 8, 8, ART_STAR + ((game.ticks >> 3) & 1u), 0,
               0);
    }
    for (u16 i = 0; i < 8; i++)
      if (!(game.coins & (1u << i)))
        sprite(f, mp_coin_x(i) / 4 - game.camera_x - 4,
               mp_coin_y(i) / 4 - game.camera_y + 40, 8, 8,
               ART_COIN + ((game.ticks >> 4) & 1u), 0, 0);
  }
  if (game.complete)
    for (u16 i = 0; i < 4; i++) {
      MpPlayer const *p = &game.players[i];
      if (!p->active || !p->hp)
        continue;
      s16 x = p->x / 4 - game.camera_x, y = p->y / 4 - game.camera_y + 40;
      u16 phase = (game.ticks + i * 7u) & 31u;
      sprite(f, x - 20 + (s16)phase,
             y - 28 - (s16)((game.ticks + i * 3u) & 15u), 8, 8,
             ART_STAR + ((game.ticks >> 3) & 1u), 0, 0);
      sprite(f, x + 16 - (s16)phase, y - 8 - (s16)((game.ticks + i * 5u) & 15u),
             8, 8, ART_COIN + ((game.ticks >> 4) & 1u), 0, 0);
    }
  __asm__ volatile("" ::: "memory");
  published = stage + 1u;
  stage ^= 1u;
}
void coop_initialize(void) {
  lab_display_off();
  lab_clear();
  lab_upload_work(res_font, to_vdp_addr(0x400) | VRAM_W, 96 * 16);
  lab_upload_work(res_actors, to_vdp_addr(ACTOR * 32u) | VRAM_W,
                  ART_COUNT * 16u);
  lab_upload_work(res_ground, to_vdp_addr(GROUND * 32u) | VRAM_W, 16 * 16);
  bios_vdp_update_flags = 0;
  reg(11, 0);
  reg(17, 0);
  reg(18, 6);
  reg(7, 2);
  vdp_ctrl_32 = CRAM_W;
  for (u16 i = 0; i < 64; i++) {
    u16 c = (res_palettes[i * 2u] << 8) | res_palettes[i * 2u + 1];
    bios_palette[i] = c;
    vdp_data_16 = c;
  }
  for (u16 y = 0; y < 32; y++) {
    vdp_ctrl_32 = to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA + y * 128u) | VRAM_W;
    for (u16 x = 0; x < 64; x++)
      vdp_data_16 = 0;
    vdp_ctrl_32 = to_vdp_addr(BIOS_VDP_DEFAULT_PLANEB + y * 128u) | VRAM_W;
    for (u16 x = 0; x < 64; x++) {
      u16 t = mp_solid(x >> 1, y >> 1) ? 2u + (x & 1u) + (y & 1u) * 2u
              : ((y >= 18 && y <= 21) || (x >= 30 && x <= 33))
                  ? 6u + (x & 1u) + (y & 1u) * 2u
              : ((x * 3u + y * 5u) & 31u) == 1 ? 1
                                               : 0;
      vdp_data_16 = GROUND + t;
    }
  }
}
static void title_page(void) {
  /* Only called at an actual title/play transition with no pending frame. */
  lab_display_off();
  for (u16 y = 6; y < 28; y++) {
    vdp_ctrl_32 = to_vdp_addr(BIOS_VDP_DEFAULT_PLANEA + y * 128u) | VRAM_W;
    for (u16 x = 0; x < 64; x++)
      vdp_data_16 = game.started ? 0 : 0x8020;
  }
  if (!game.started) {
    lab_line("LIGHT THE EIGHT GARDEN LANTERNS", 4, 8);
    lab_line("KEEP YOUR PARTY TOGETHER", 6, 10);
    lab_line("D-PAD WALK    A STUN MOTHS", 5, 13);
    lab_line("START PAUSE   HOLD START+B LEAVE", 3, 15);
    lab_line("EACH KEEPER RETURNS INDEPENDENTLY", 3, 18);
    lab_line("WHITE BAR: STUN RECHARGES IN 3S", 4, 20);
    lab_line("C: FOUR-PLAYER CPU TOUR", 7, 24);
  }
  title_drawn = game.started;
}
void coop_begin_probe(void) {
  mp_demo(&game);
  for (u16 i = 0; i < 4; i++)
    game.players[i].hurt = 0;
  hz = 60;
  stage = 0;
  published = 0;
}
void main(void) {
  disable_interrupts();
  system_install();
#if VIDEO == PAL
  hz = 50;
#else
  hz = 60;
#endif
  mp_init(&game);
  for (u16 i = 0; i < 8; i++)
    in_reset(&inputs[i]);
#ifdef COOP_AUTOTEST
  /* CPU tour is presentation; regression execution belongs to the harness. */
  mp_demo(&game);
#endif
  coop_initialize();
  title_page();
  enable_interrupts();
  coop_publish();
  do {
    lab_wait();
  } while (published);
  lab_display_on();
  u16 previous = main_ticks;
  for (;;) {
    if (previous == main_ticks)
      lab_wait();
    u16 delta = (u16)(main_ticks - previous);
    previous = main_ticks;
    input_poll(samples);
    u16 hits[8], all = 0;
    for (u16 i = 0; i < 8; i++) {
      in_update(&inputs[i], &samples[i], hz);
      hits[i] = inputs[i].pressed;
      all |= hits[i];
    }
    if (!game.started && (all & IN_C))
      mp_demo(&game);
    else if (game.demo && (all & (IN_C | IN_START)))
      mp_init(&game);
    else if (game.complete && (all & IN_A)) {
      mp_restart(&game);
    } else
      mp_advance(&game, delta >= hz ? 300 : delta * (hz == 50 ? 6u : 5u),
                 samples, hits);
    lab_sound(game.ticks, game.events, game.paused || !game.started);
    if (published)
      continue;
    u16 changed = title_drawn != game.started;
    if (changed)
      title_page();
    coop_publish();
    if (changed) {
      do {
        lab_wait();
      } while (published);
      lab_display_on();
      previous = main_ticks;
    }
  }
}

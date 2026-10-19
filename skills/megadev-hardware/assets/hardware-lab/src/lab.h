#ifndef PROJECT_LAB_H
#define PROJECT_LAB_H
#include <types.h>
#include "usb_input.h"
void lab_video_init(void);
void lab_present(void);
void lab_heartbeat(u16 tick);
void lab_usb_poll(void);
const UiPort *lab_input_port(u16 port);
u16 lab_input_pressed(void);
void lab_clear_region(u16 x,u16 y,u16 width,u16 height);
void lab_text(char const *s,u16 x,u16 y,u16 palette);
void lab_line(char const *s,u16 x,u16 y);
void lab_hex(u16 value,u16 x,u16 y);
void lab_screen(char const *title);
void lab_wait(void);
void lab_display_on(void);
void lab_display_off(void);
void lab_font(void);
void lab_clear(void);
void lab_print(char const *s,u32 pos);
extern volatile u32 main_ticks;
extern volatile u16 room_runs;
extern volatile u16 room_pass;
#endif

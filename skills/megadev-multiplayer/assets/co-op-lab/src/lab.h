#ifndef PROJECT_LAB_H
#define PROJECT_LAB_H
#include <types.h>
void lab_line(char const *s,u16 x,u16 y);
void lab_hex(u16 value,u16 x,u16 y);
void lab_screen(char const *title);
void lab_wait(void);
void lab_display_on(void);
void lab_display_off(void);
void lab_font(void);
void lab_clear(void);
void lab_upload_word(void const *source,u32 command,u32 words);
void lab_upload_work(void const *source,u32 command,u32 words);
void lab_upload_word(void const *source,u32 command,u32 words);
void lab_print(char const *s,u32 pos);
extern volatile u16 main_ticks;
extern volatile u16 room_runs;
#endif

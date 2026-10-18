#ifndef SCALING_LAB_H
#define SCALING_LAB_H
/* Fixed teaching layout, offsets relative to 2M Word RAM. */
typedef unsigned short ScU16;
typedef signed short ScS16;
typedef unsigned int ScU32;
typedef signed int ScS32;
#define SC_WIDTH 128u
#define SC_HEIGHT 128u
#define SC_WORDS 4096u
#define SC_STAMPS 0x00000u
#define SC_MAP 0x09000u
#define SC_TRACE 0x09400u
#define SC_IMAGE 0x10000u
#define SC_BUSY 0x8000u
#define SC_UPLOAD_WORDS 2048u
#define SC_TIMEOUT 1500u /* 300 Hz units, five seconds; timeout never frees a lease. */
typedef struct {ScU16 x,y,dx,dy;} ScLine;
typedef struct {ScU16 mode,angle,zoom,motion;} ScRequest;
_Static_assert(sizeof(ScLine)==8,"ASIC trace line must contain four words");
_Static_assert(SC_MAP%512==0 && SC_TRACE%8==0 && SC_IMAGE%32==0,"ASIC address alignment");
_Static_assert(32896<=SC_MAP && SC_MAP+512<=SC_TRACE && SC_TRACE+SC_HEIGHT*8<=SC_IMAGE,"Word RAM regions overlap");
_Static_assert(SC_IMAGE+SC_WORDS*2<=262144 && SC_WIDTH*SC_HEIGHT/4==SC_WORDS,"Image extent");
int scale_valid(ScRequest const *r);
int scale_trace(ScLine *out,ScU16 capacity,ScRequest const *r);
ScS16 scale_sine(ScU16 angle);
/* Project adapters, not Megadev SDK functions. */
int scale_start(ScRequest const *r);
void scale_main_tick(ScU16 elapsed);
void scale_sub_tick(void);
extern ScU16 scale_phase,scale_error,scale_completed,scale_generation;
extern volatile ScU16 scale_upload,scale_last_mode;
extern ScU16 scale_pixels[SC_WORDS];
#endif

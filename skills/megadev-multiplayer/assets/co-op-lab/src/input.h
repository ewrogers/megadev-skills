#ifndef INPUT_LAB_INPUT_H
#define INPUT_LAB_INPUT_H
#ifdef _WIN32
#define IN_API __declspec(dllexport)
#else
#define IN_API
#endif
/* Low byte matches Megadev PAD_*; upper bits are this example's extension. */
typedef unsigned char InU8;
typedef unsigned short InU16;
typedef signed short InS16;
#define IN_UP 1
#define IN_DOWN 2
#define IN_LEFT 4
#define IN_RIGHT 8
#define IN_B 16
#define IN_C 32
#define IN_A 64
#define IN_START 128
#define IN_X 256
#define IN_Y 512
#define IN_Z 1024
#define IN_MODE 2048
#define IN_SLOTS 8
#define IN_WAIT_LIMIT 128
#define IN_PACKET_READ_BUDGET 256
/* Type and freshness are separate: an overflow is still a mouse, not a pad. */
enum {IN_NONE,IN_PAD3,IN_PAD6,IN_MOUSE,IN_UNKNOWN};
enum {IN_OK,IN_ABSENT,IN_TIMEOUT,IN_BAD_PACKET,IN_OVERFLOW,IN_BUS_BUSY};
typedef struct {InU16 kind,status,held;InS16 dx,dy;} InSample;
typedef struct {
    InU16 held,pressed,released,repeat,last_dir,repeat_time,kind,initialized,player;
    InS16 x,y;
} InSlot;
typedef struct {
    void *context;
    InU8 (*read)(void *context,InU16 port);
    void (*write)(void *context,InU16 port,InU16 control,InU8 value);
    void (*settle)(void *context);
} InIo;
void in_clear(InSample *sample,InU16 status);
int in_pad(InU8 const steps[8],InSample *sample);
int in_mouse(InU8 const nibbles[6],InSample *sample);
int in_team(InU8 const *nibbles,InU16 length,InSample slots[4]);
int in_poll_port(InIo const *io,InU16 port,InSample slots[4]);
int in_poll_ea(InIo const *io,InSample slots[8]);
IN_API void in_reset(InSlot *slot);
IN_API void in_update(InSlot *slot,InSample const *sample,InU16 hz);
InU16 in_join(InSlot slots[8],InU16 index);
#endif

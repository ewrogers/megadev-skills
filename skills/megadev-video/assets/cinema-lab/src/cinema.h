#ifndef CINEMA_H
#define CINEMA_H
/* Application format/policy, not Megadev APIs. All wire integers are big endian. */
typedef unsigned char V8;
typedef unsigned short V16;
typedef unsigned int V32;
#define CI_WIDTH 160u
#define CI_HEIGHT 96u
#define CI_BYTES 7680u
#define CI_FRAMES 48u
#define CI_GROUP 4u
#define CI_FILE_BYTES 32768u
#define CI_PERIOD 30u /* 300 Hz units: nominal 10 images/sec, not a guarantee. */
#define CI_BATCH 4096u
#define CI_SLOT0 0x100u
#define CI_SLOT1 0x200u
#define CI_TIMEOUT 9000u
#define CI_STAGE 0x28000u /* Sub PRG RAM staging: 28000..2FFFF, outside code/stack. */
enum {CI_OK=1,CI_MISSING,CI_IO,CI_FORMAT,CI_CANCEL,CI_TIMEOUT_ERROR};
typedef struct {V16 frame,visible,uploaded,slot,clock,late,presented,done,paused;} Cinema;
#if defined(_WIN32)
#define CI_API __declspec(dllexport)
#else
#define CI_API
#endif
CI_API V16 ci_word(volatile V8 const *p);
CI_API V16 ci_header(volatile V8 const *p,V32 length,V16 first);
CI_API V16 ci_sum(volatile V8 const *p,V16 size);
CI_API void ci_init(Cinema *s);
CI_API V16 ci_begin(Cinema *s,V16 frame);
CI_API V16 ci_upload(Cinema *s,V16 bytes);
CI_API V16 ci_due(Cinema const *s);
CI_API V16 ci_present(Cinema *s);
CI_API void ci_tick(Cinema *s,V16 elapsed);
void cinema_sub_tick(V16 now);
V32 stream_file_size(char const *name);
#endif

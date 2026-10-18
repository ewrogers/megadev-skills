#ifndef STREAM_LAB_H
#define STREAM_LAB_H
/* Project-owned example contracts, not Megadev APIs. */
typedef unsigned short S16;
typedef unsigned int S32;
#define STREAM_CAP 32768u
#define STREAM_TILES 640u
#define STREAM_MAP 800u
#define STREAM_CHUNK 256u
#define STREAM_TIMEOUT 9000u /* 30 seconds in 300 Hz units, not a drive guarantee. */
enum { ST_IDLE,ST_GRANT,ST_READ,ST_RETIRE,ST_VALIDATE,ST_UPLOAD,ST_DRAIN,ST_FAULT };
enum { SR_OK=1,SR_CANCEL=2,SR_MISSING=3,SR_SIZE=4,SR_IO=5,SR_FORMAT=6,SR_TIMEOUT=7 };
typedef struct {
    S32 length,offset,crc,wanted,map_offset;
    S16 tiles,done,valid;
} StreamImage;
S16 stream_word(volatile unsigned char const *p);
S32 stream_rounded(S32 bytes,S32 capacity);
int stream_image_begin(StreamImage *v,volatile unsigned char const *data,S32 length);
int stream_image_step(StreamImage *v,volatile unsigned char const *data,S16 budget);
extern S16 stream_phase,stream_ready,stream_asset,stream_result,stream_pass,stream_dropped,stream_errors,stream_timeouts;
extern S16 stream_uploaded,stream_cache_map[STREAM_MAP],stream_cache_palette[16],stream_cache_tiles;
int stream_start(S16 asset);
void stream_cancel(void);
void stream_main_tick(S16 elapsed);
void stream_upload_done(S16 bytes);
void stream_sub_tick(S16 now,S16 hz);
S32 stream_file_size(char const *name);
#endif

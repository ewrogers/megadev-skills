#ifndef SAVE_LAB_H
#define SAVE_LAB_H
typedef unsigned char Sv8;
typedef unsigned short Sv16;
typedef unsigned int Sv32;
#define SAVE_BYTES 64
enum { SV_OK,SV_SAVED,SV_LOADED,SV_RECOVERED,SV_EMPTY,SV_MEDIA,SV_IO,SV_FUTURE,SV_INVALID,SV_META,SV_VERIFY,SV_EXHAUSTED,SV_CONFLICT,SV_FORMATTED };
enum { SLOT_EMPTY,SLOT_VALID,SLOT_CORRUPT,SLOT_FUTURE,SLOT_ERROR };
typedef struct {Sv16 level,coins,flags;} SaveGame;
typedef struct {Sv32 sequence;Sv16 best,valid_mask,old_version,slots[2];} SaveSummary;
Sv32 save_crc(Sv8 const *bytes,Sv16 count);
int save_encode(Sv8 *out,SaveGame const *game,Sv32 sequence,Sv16 version);
Sv16 save_decode(Sv8 const *in,SaveGame *game,Sv32 *sequence,Sv16 *version);
Sv16 save_load(SaveGame *out,SaveSummary *summary);
Sv16 save_write(SaveGame const *game,SaveSummary *summary);
Sv16 save_damage(SaveSummary *summary);
/* Project backend returns OK / EMPTY / IO / META; never formats or deletes. */
Sv16 save_backend_read(Sv16 slot,Sv8 *out);
Sv16 save_backend_write(Sv16 slot,Sv8 const *data);
void save_sub_tick(void);
/* Op 5 erases ALL saves on the selected device; requires explicit UI confirmation. */
int save_request(Sv16 op,SaveGame const *game);
void save_main_tick(Sv16 elapsed);
extern Sv16 save_pending,save_result,save_media,save_sequence,save_fault;
extern SaveGame save_reply;
Sv16 cart_execute(Sv16 op,SaveGame const *game);
extern Sv16 save_device,cart_id;
#endif

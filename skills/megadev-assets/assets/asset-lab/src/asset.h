#ifndef ASSET_FORMAT_H
#define ASSET_FORMAT_H
/* Teaching format; fields are decoded from bytes, never cast to a packed struct. */
typedef struct AssetView {
    unsigned short kind,width,height,tiles,entries;
    unsigned int tile_bytes,total,map_offset;
} AssetView;
unsigned short asset_word(volatile unsigned char const *p);
unsigned int asset_crc(volatile unsigned char const *p,unsigned int length);
unsigned short asset_open(volatile unsigned char const *data,unsigned int available,AssetView *view);
#endif

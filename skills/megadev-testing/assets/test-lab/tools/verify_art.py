#!/usr/bin/env python3
"""Validate the Lab's teaching-format assets before generating size constants."""
import argparse
from pathlib import Path
import struct
import zlib

def validate(path,actor=False):
    data=Path(path).read_bytes()
    if len(data)<64 or data[:4]!=b'MDA1':raise ValueError('Invalid MDA header')
    version,kind,width,height,tiles,entries,tile_bytes,total,crc,reserved=struct.unpack('>6H4I',data[4:32])
    if version!=1 or reserved or total!=len(data) or tile_bytes!=tiles*32 or zlib.crc32(data[32:])!=crc:
        raise ValueError('Invalid MDA size/checksum')
    if any(color&0xF111 for color in struct.unpack('>16H',data[32:64])):raise ValueError('Invalid VDP palette')
    if actor:
        if (kind,width,height,entries,tiles)!=(1,2,3,2,12) or len(data)!=64+tile_bytes:raise ValueError('Expected two 16x24 actor frames')
    else:
        if (kind,width,height,entries)!=(0,40,20,800) or not 0<tiles<=640 or len(data)!=64+tile_bytes+1600:
            raise ValueError('Expected 320x160 area within its 640-tile VRAM slot')
        if any(cell&0xE000 or cell&0x7FF>=tiles for cell in struct.unpack('>800H',data[64+tile_bytes:])):
            raise ValueError('Invalid tilemap reference')
        if ((len(data)+2047)//2048)*2048>32768:raise ValueError('Rounded CD read exceeds reserved buffer')
    return len(data)

def verify(root):
    return {name:validate(root/'res'/f'{name}.mda',name=='actor') for name in ['garden','cavern','actor']}
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--root',type=Path,default=Path('.'))
    args=parser.parse_args();sizes=verify(args.root)
    out=args.root/'build/stream_assets.h';out.parent.mkdir(exist_ok=True)
    out.write_text('/* Generated from validated assets. */\n'+''.join(f'#define STREAM_{name.upper()}_BYTES {size}u\n' for name,size in sizes.items()))

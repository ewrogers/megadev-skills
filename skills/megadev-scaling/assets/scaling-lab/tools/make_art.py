#!/usr/bin/env python3
"""Original native-resolution brass observatory mosaic; no resampling/quantization.

Writes column-major 16x16 ASIC stamps, an explicit blank stamp zero and a PNG
reference. Uses only the standard library so standalone builds need no art SDK.
"""
from pathlib import Path
import struct
import zlib
ROOT=Path(__file__).resolve().parents[1]
COLORS=[0x0000,0x0220,0x0442,0x0664,0x0886,0x0CCA,0x0EEE,0x0026,0x004A,0x008E,0x00CE,0x0224,0x0468,0x068A,0x08CC,0x0CEE]
def art():
    pixels=[]
    for y in range(256):
        row=[]
        for x in range(256):
            dx=x-128;dy=y-128;r=dx*dx+dy*dy
            c=7 if (y&31)<2 else 8
            if ((x+y*3)&63)==0:c=9
            if r<116*116:
                c=2 if ((x//16+y//16)&1) else 3
                if x%16==0 or y%16==0:c=1
            if 108*108<r<116*116:c=4 if (x+y)&1 else 5
            if 105*105<r<=108*108:c=11
            if abs(dx)<11 or abs(dy)<11:c=12 if ((x+y)&7) else 13
            if r<58*58:c=11
            if 51*51<r<58*58:c=10 if (x+y)&1 else 5
            if r<49*49:c=7 if ((x//8+y//8)&1) else 8
            if 24*24<r<27*27:c=14
            if (abs(dx)<3 or abs(dy)<3) and r<49*49:c=5
            if abs(dx)+abs(dy)<18:c=6 if dx<0 else 10
            if abs(dx)+abs(dy)<10:c=9 if dy<0 else 15
            if x%64 in (13,14) and y%64 in range(12,18):c=5
            row.append(c)
        pixels.append(row)
    # Four clipped-corner pavilions with crisp roofing and warm lanterns.
    for ox,oy in [(55,55),(183,55),(55,183),(183,183)]:
        for yy in range(-15,17):
            for xx in range(-17,18):
                if abs(xx)+max(0,abs(yy)-9)>19:continue
                c=1
                if abs(xx)<15 and abs(yy)<13:c=12
                if abs(xx)<12 and yy<-2:c=3 if yy&3 else 4
                if abs(xx)<3 and yy>=1:c=11
                if abs(xx) in (9,10) and 1<=yy<=8:c=10
                if yy==14:c=13
                pixels[oy+yy][ox+xx]=c
    return pixels
def stamp_bytes(p):
    data=bytearray(128) # Index zero is always transparent on the ASIC.
    for sy in range(0,256,16):
        for sx in range(0,256,16):
            for col in (0,8):
                for y in range(16):
                    for x in range(0,8,2):data.append(p[sy+y][sx+col+x]*16+p[sy+y][sx+col+x+1])
    return bytes(data)
def png(p):
    def chunk(n,b):return struct.pack('>I',len(b))+n+b+struct.pack('>I',zlib.crc32(n+b))
    rgb=[]
    for c in COLORS:rgb+= [((c>>1)&7)*255//7,((c>>5)&7)*255//7,((c>>9)&7)*255//7]
    scan=b''.join(b'\0'+bytes(row) for row in p)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',256,256,8,3,0,0,0))+chunk(b'PLTE',bytes(rgb))+chunk(b'IDAT',zlib.compress(scan))+chunk(b'IEND',b'')
def main():
    out=ROOT/'res';out.mkdir(exist_ok=True);p=art()
    (out/'stamps.bin').write_bytes(stamp_bytes(p));(out/'palette.bin').write_bytes(struct.pack('>16H',*COLORS));(out/'observatory.png').write_bytes(png(p))
if __name__=='__main__':main()

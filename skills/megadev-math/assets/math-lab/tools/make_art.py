#!/usr/bin/env python3
"""Native dots/grid and a reproducible quarter-wave integer lookup table."""
import math
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
def tile(p):return bytes((p[y][x]<<4)|p[y][x+1] for y in range(8) for x in range(0,8,2))
def build():
 actors=[]
 for n in range(4):
  p=[[0]*8 for _ in range(8)]
  for y in range(8):
   for x in range(8):
    d=(2*x-7)**2+(2*y-7)**2
    if n==3:
     if (x in (3,4) or y in (3,4)) and d<45:p[y][x]=1 if d<20 else 8
    elif d<42:p[y][x]=2 if d>28 else 1 if x+y<7 else (6 if n==0 else 9 if n==1 else 12)
  actors.append(tile(p))
 ground=tile([[4 if (x==0 and y==0) else 3 for x in range(8)] for y in range(8)])
 palettes=[[0,0xEEE,0x222,0x422]+[0]*12,[0,0xEEE,0x222,0x422,0x644,0xA64,0xEC6,0x862,0xECA,0x6CE,0x48A,0x84A,0xCAE,0x68C,0xCEE,0x8AC],[0,0xEEE,0x222,0x200,0x422]+[0]*11,[0]*16]
 return {'actors.bin':b''.join(actors),'ground.bin':ground,'palettes.bin':struct.pack('>64H',*(c for p in palettes for c in p))}
def generate():
 for name,data in build().items():(ROOT/'res'/name).write_bytes(data)
 values=[int(math.floor(math.sin(math.pi*i/512)*256+0.5)) for i in range(257)]
 text='#ifndef MM_SINE_TABLE_H\n#define MM_SINE_TABLE_H\nstatic const MmU16 mm_quarter[257]={\n'+',\n'.join(','.join(map(str,values[i:i+16])) for i in range(0,257,16))+'\n};\n#endif\n'
 (ROOT/'src/sine_table.h').write_text(text)
if __name__=='__main__':generate();print('Native Math Lab art and 514-byte quarter-wave table generated')

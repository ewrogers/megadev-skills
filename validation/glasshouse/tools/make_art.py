#!/usr/bin/env python3
"""Original native-resolution greenhouse art, deduplicated to bounded VDP tiles."""
from pathlib import Path
import struct
import zlib
ROOT=Path(__file__).resolve().parents[1]
def tile(p):return bytes((p[y][x]<<4)|p[y][x+1] for y in range(8) for x in range(0,8,2))
def mda(kind,w,h,entries,patterns,mapping,palette):
 body=struct.pack('>16H',*palette)+patterns+mapping
 return b'MDA1'+struct.pack('>6H4I',1,kind,w,h,len(patterns)//32,entries,len(patterns),32+len(body),zlib.crc32(body),0)+body
WORLD=[0x0222,0x0CEE,0x0244,0x0466,0x0688,0x08AC,0x0246,0x0468,0x06AC,0x0242,0x0262,0x0484,0x08C8,0x0622,0x0A44,0x0E86]
NIGHT=[0x0222,0x0CEE,0x0442,0x0664,0x0886,0x0AA8,0x0246,0x0468,0x06AC,0x0242,0x0262,0x0484,0x08C8,0x0622,0x0A44,0x0E86]
ACTOR=[0,0x0CEE,0x0222,0x0444,0x0468,0x08AC,0x0ACE,0x0644,0x0866,0x0CAA,0x0442,0x0664,0x0AA6,0x0CC8,0x046A,0x08CE]
UI=[0,0x0CEE,0x0222,0x0444,0x0666,0x0688,0x08AA,0x0ACA,0x0CE8,0x0642,0x0864,0x0CA6,0x044A,0x068E,0x0ACC,0x0EEE]
def picture(area):
 p=[[2]*320 for _ in range(160)]
 def rect(x,y,w,h,c):
  for j in range(max(0,y),min(160,y+h)):
   for i in range(max(0,x),min(320,x+w)):p[j][i]=c
 def rim(x,y,w,h,c=7):
  rect(x,y,w,h,0);rect(x+1,y+1,w-2,h-2,c);rect(x+3,y+3,w-6,h-6,2)
 # Floor grid, low-contrast repeated paving.
 for y in range(160):
  for x in range(320):p[y][x]=3 if y%16==0 or x%32==0 else 2
 rect(0,0,320,18,0)
 for x in range(8,280,48):
  rim(x,0,40,24,5);rect(x+4,2,32,17,13)
  for y in range(2,19):
   for dx in range(4,36):
    if (dx+y)%4==0:p[y][x+dx]=14
  rect(x+19,1,2,20,4)
 for x in [0,312]:rect(x,18,8,142,6);rect(x+2,18,2,142,8)
 # Central beds/workbenches, exact solids in world.c (world y = image y + 32).
 for y in [32,112]:
  rim(104,y,80,32,7);rect(108,y+4,72,22,9 if not area else 3)
  if not area:
   for x in range(112,176,16):
    rect(x,y+15,10,7,6);rect(x+3,y+5,3,12,10);rect(x,y+8,9,3,11);rect(x+2,y+5,6,2,12)
  else:
   rect(112,y+8,64,3,5)
   for x in [120,144,168]:rect(x,y+5,5,16,7);rect(x+1,y+6,2,8,8)
 # Interaction fixtures, door with a bright threshold.
 if area:
  rim(40,32,40,32,5);rect(47,36,26,21,10);rect(52,38,16,5,8);rect(54,47,12,7,0);rect(59,45,2,9,1)
  rim(224,96,32,32,5);rect(232,102,16,16,8);rect(237,99,5,22,1);rect(229,107,22,5,1)
  rim(8,72,24,40,8);rect(12,75,16,34,9);rect(8,110,24,3,1)
 else:
  rim(216,64,32,32,7);rect(222,82,20,9,6);rect(231,68,2,16,10)
  for dx,dy in [(0,-5),(-5,0),(5,0),(0,5)]:rect(229+dx,70+dy,6,6,14)
  rect(229,70,6,6,1)
  rim(280,72,24,40,8);rect(284,75,16,34,9);rect(280,110,24,3,1)
 # Deduplication preserves native pixels; no resampling or palette matching.
 seen={};patterns=bytearray();cells=[]
 for ty in range(20):
  for tx in range(40):
   data=tile([row[tx*8:tx*8+8] for row in p[ty*8:ty*8+8]])
   if data not in seen:seen[data]=len(seen);patterns.extend(data)
   cells.append(seen[data])
 if len(seen)>640:raise ValueError('World exceeds its 640-tile slot')
 return mda(0,40,20,800,bytes(patterns),struct.pack('>800H',*cells),WORLD if not area else NIGHT)
def actor():
 patterns=bytearray()
 for facing in range(4):
  for frame in range(2):
   p=[[0]*16 for _ in range(24)]
   def r(x,y,w,h,c):
    for yy in range(y,y+h):
     for xx in range(x,x+w):p[yy][xx]=c
   r(4,1,8,8,2);r(5,2,6,6,5);r(3,1,10,3,4);r(2,3,12,2,14)
   if facing==1:r(5,5,1,1,2);r(10,5,1,1,2)
   elif facing in (2,3):r(4 if facing==2 else 10,5,2,2,2)
   else:r(5,4,6,4,4)
   r(3,9,10,10,2);r(4,9,8,8,11);r(6,11,4,6,12);r(3,11,2,7,5);r(11,11,2,7,5)
   r(5,18,3,4,3);r(9,18,3,4,3);r(4,21-frame,4,2,2);r(9,20+frame,4,2,2)
   for tx in range(2):
    for ty in range(3):patterns+=tile([row[tx*8:tx*8+8] for row in p[ty*8:ty*8+8]])
 return mda(1,2,3,8,bytes(patterns),b'',ACTOR)
def panels():
 out=bytearray()
 for sy in range(3):
  for sx in range(3):
   p=[[2]*8 for _ in range(8)]
   for y in range(8):
    for x in range(8):
     if (sx==0 and x<2) or (sx==2 and x>=6) or (sy==0 and y<2) or (sy==2 and y>=6):p[y][x]=5 if (x+y)&1 else 7
   out+=tile(p)
 return bytes(out)
def generate(root=ROOT):
 res=root/'res';res.mkdir(exist_ok=True)
 for name,data in {'garden.mda':picture(0),'cavern.mda':picture(1),'actor.mda':actor(),'ui.bin':panels(),'ui.pal':struct.pack('>16H',*UI)}.items():(res/name).write_bytes(data)
if __name__=='__main__':generate()

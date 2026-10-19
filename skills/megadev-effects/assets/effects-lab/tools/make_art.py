#!/usr/bin/env python3
"""Original native-resolution observatory art; no rescaling or external inputs."""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
def tile(p):return bytes((p[y][x]<<4)|p[y][x+1] for y in range(8) for x in range(0,8,2))
def build():
 image=[[1]*320 for _ in range(224)]
 def rect(x,y,w,h,c):
  for yy in range(max(0,y),min(224,y+h)):
   for xx in range(max(0,x),min(320,x+w)):image[yy][xx]=c
 def line(x,y,x2,y2,c):
  n=max(abs(x2-x),abs(y2-y),1)
  for i in range(n+1):rect(x+(x2-x)*i//n,y+(y2-y)*i//n,1,1,c)
 for y in range(32,224):
  for x in range(320):
   image[y][x]=1 if y<105 else 2 if y<152 else 3
   if y<128 and ((x*37+y*71)%997==0):image[y][x]=7
 # Restrained dither only at the two sky ramps.
 for y0,c in [(99,2),(146,3)]:
  for y in range(y0,y0+6):
   for x in range(320):
    if (x+y)&3 < (y-y0)//2:image[y][x]=c
 for x in range(320):
  height=131+abs((x+30)%122-61)//3
  rect(x,height,1,224-height,4)
 for x in [12,65,272,307]:
  rect(x,106,3,92,5)
  for k in range(6):line(x+1,118+k*12,x-13,128+k*12,5);line(x+1,118+k*12,x+15,130+k*12,5)
 # Moon and two observatory towers.
 for y in range(44,78):
  for x in range(239,273):
   if (x-256)**2+(y-61)**2<260:image[y][x]=8 if (x-253)**2+(y-58)**2<210 else 7
 for x,y,w in [(34,112,36),(244,124,32)]:
  rect(x,y,w,79,6);rect(x+3,y+3,w-6,74,5)
  for yy in range(y+14,186,18):
   for xx in range(x+8,x+w-5,12):rect(xx,yy,4,7,9);rect(xx+1,yy+1,2,4,10)
  for i in range(w//2):rect(x+i,y-i//2,w-2*i,1,6)
 for y in range(190,224):
  for x in range(320):image[y][x]=11 if y%12==0 or (x+((y//12)&1)*16)%32==0 else 6 if y%12==1 else 5
 rect(0,188,320,2,7)
 patterns=[];lookup={};mapping=[]
 for ty in range(28):
  for tx in range(40):
   b=tile([image[ty*8+y][tx*8:tx*8+8] for y in range(8)])
   if b not in lookup:lookup[b]=len(patterns);patterns.append(b)
   mapping.append(0x4000|(0x100+lookup[b]))
 assert len(patterns)<=800
 # One 16x16 lantern in hardware column-major order, then nine 8px effects.
 hero=[[0]*16 for _ in range(16)]
 for y in range(16):
  for x in range(16):
   if 4<=x<=11 and 4<=y<=12:hero[y][x]=2 if x in (4,11) else 3 if y in (4,12) else 1 if x<8 else 4
   if y in (2,3,13,14) and 3<=x<=12:hero[y][x]=3
   if y<3 and x in (6,9):hero[y][x]=2
 actors=[tile([row[x:x+8] for row in hero[y:y+8]]) for x in (0,8) for y in (0,8)]
 for k in range(9):
  p=[[0]*8 for _ in range(8)]
  for y in range(8):
   for x in range(8):
    d=(2*x-7)**2+(2*y-7)**2
    if k<3 and d<(45,32,14)[k]:p[y][x]=1 if d<10 else 4 if d<25 else 5 if k<2 else 6
    elif k in (3,4) and d<(40,55)[k-3]:p[y][x]=7 if d<22 else 8
    elif k in (5,6) and d<(14,7)[k-5]:p[y][x]=4 if k==5 else 5
    elif k==7 and x==5-y//3:p[y][x]=9 if y<3 else 10
    elif k==8 and ((x in (3,4) and 1<y<6) or (y in (3,4) and 1<x<6)):p[y][x]=1 if d<12 else 9
  actors.append(tile(p))
 palettes=[[0x020,0xEEE,0x222]+[0]*13,[0]*16,[0x020,0x200,0x422,0x644,0x642,0x422,0x864,0xCA8,0xECA,0x08C,0x4CE,0xA86]+[0]*4,[0,0xEEE,0x222,0x48A,0x6CE,0x28E,0x04A,0xA86,0x864,0xECA,0xA86]+[0]*5]
 return {'actors.bin':b''.join(actors),'ground.bin':b''.join(patterns),'map.bin':struct.pack('>1120H',*mapping),'palettes.bin':struct.pack('>64H',*(c for p in palettes for c in p))}
def generate():
 for name,data in build().items():(ROOT/'res'/name).write_bytes(data)
if __name__=='__main__':generate();print('Native observatory and resident effect atlas generated')

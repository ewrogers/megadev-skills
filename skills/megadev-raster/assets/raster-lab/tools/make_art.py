#!/usr/bin/env python3
"""Original native-pixel city/reflection and shared two-camera garden."""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
def tile(p):return bytes((p[y][x]<<4)|p[y][x+1] for y in range(8) for x in range(0,8,2))
def pack(image,palette):
 patterns=[];lookup={};mapping=[]
 for ty in range(32):
  for tx in range(64):
   b=tile([image[ty*8+y][tx*8:tx*8+8] for y in range(8)])
   if b not in lookup:lookup[b]=len(patterns);patterns.append(b)
   mapping.append((palette<<13)|(0x100+lookup[b]))
 assert len(patterns)<=768,'Patterns would collide with reflection table at 8000'
 return b''.join(patterns),mapping

def background(demo):
 image=[[1]*512 for _ in range(256)]
 def rect(x,y,w,h,c):
  for yy in range(max(0,y),min(256,y+h)):
   for xx in range(max(0,x),min(512,x+w)):image[yy][xx]=c
 def line(x,y,x2,y2,c):
  n=max(abs(x2-x),abs(y2-y),1)
  for i in range(n+1):rect(x+(x2-x)*i//n,y+(y2-y)*i//n,1,1,c)
 if demo=='water':
  for y in range(256):
   for x in range(512):
    image[y][x]=1 if y<65 else 2 if y<104 else 3
    if y<100 and (x*29+y*73)%997==0:image[y][x]=8
  for y in range(48,80):
   for x in range(357,389):
    if (x-373)**2+(y-64)**2<240:image[y][x]=9 if (x-370)**2+(y-61)**2<186 else 8
  for x in range(512):rect(x,99+abs(x%110-55)//4,1,157,4)
  for x,y,w,h in [(18,114,48,100),(100,94,52,120),(187,107,44,106),(270,88,56,128),(363,115,40,105),(439,99,52,116)]:
   rect(x,y,w,h,5);rect(x+3,y+3,w-6,h-3,6);rect(x+5,y+6,w-10,h-6,5)
   for yy in range(y+15,y+h-8,19):
    for xx in range(x+9,x+w-5,13):rect(xx,yy,5,9,10);rect(xx+1,yy+1,3,5,11)
   for i in range(w//2):rect(x+i,y-i//2,w-2*i,1,7)
   rect(x+w//2-1,y-w//4-10,2,12,8)
  rect(0,216,512,3,8)
  for y in range(219,256):
   for x in range(512):image[y][x]=7 if y%12==0 or (x+(y//12%2)*16)%32==0 else 6
 else:
  for y in range(256):
   for x in range(512):
    image[y][x]=3 if ((x//8+y//8)&1) else 4
    if (x*13+y*7)%211==0:image[y][x]=5
  for x in [72,240,416]:rect(x,0,32,256,6);rect(x+2,0,28,256,7)
  for y in [56,176]:rect(0,y,512,28,6);rect(0,y+2,512,24,7)
  for x,y in [(20,16),(132,100),(320,20),(452,104),(268,202)]:
   rect(x,y,42,34,2);rect(x+2,y+2,38,30,10);rect(x+4,y+6,34,24,11)
   rect(x+13,y+17,10,15,2);rect(x+5,y+9,6,6,8);rect(x+29,y+9,6,6,8)
   for i in range(8):rect(x-4+i,y-i,50-2*i,2,12)
  for x,y in [(183,29),(366,137),(34,132),(478,213)]:
   for yy in range(y-14,y+15):
    for xx in range(x-22,x+23):
     if (xx-x)**2+(yy-y)**2*2<400 and 0<=xx<512 and 0<=yy<256:image[yy][xx]=9 if (xx+yy)&7 else 8
  for x,y in [(120,34),(190,133),(350,210),(460,40),(48,220)]:
   rect(x,y,3,18,6)
   for yy in range(y-12,y+4):
    for xx in range(x-12,x+14):
     if (xx-x)**2+(yy-y+5)**2<120 and 0<=xx<512 and 0<=yy<256:image[yy][xx]=5 if xx<x else 4
 return image

def build():
 out={}
 for demo in ['water','split']:
  pixels,mapping=pack(background(demo),1);out[demo+'_ground.bin']=pixels
  out[demo+'_map.bin']=struct.pack('>2048H',*mapping)
  if demo=='water':
   reflected=[(mapping[(31-y)*64+x]&0x7FF)|0x5000 for y in range(32) for x in range(64)]
   out['reflection_map.bin']=struct.pack('>2048H',*reflected)
 actors=[]
 for n in range(2):
  p=[[0]*16 for _ in range(16)]
  for y in range(16):
   for x in range(16):
    d=(2*x-15)**2+(2*y-15)**2
    if d<190:p[y][x]=2 if d>140 else (4+n*2) if x+y<16 else (5+n*2)
    if 5<=x<=7 and 4<=y<=6:p[y][x]=1
  for x in (0,8):
   for y in (0,8):actors.append(tile([r[x:x+8] for r in p[y:y+8]]))
 for frame in range(5):
  p=[[0]*32 for _ in range(8)]
  for y in range(8):
   for x in range(32):
    if frame==4:p[y][x]=5 if y in (0,7) else 4 if y in (1,6) else 3
    else:
     crest=1+((x*7+frame*11)&3)
     if y>=2:p[y][x]=3 if y>5 else 4 if (x+2*y+frame)&3 else 1
     elif y>=crest-2:p[y][x]=1 if (x+frame)&1 else 5
  for x in range(0,32,8):actors.append(tile([r[x:x+8] for r in p]))
 out['actors.bin']=b''.join(actors)
 dry=[0x200,0x200,0x422,0x644,0x642,0x422,0x864,0xA86,0xCCA,0xECA,0x08C,0x4CE,0x66A,0xACE,0x888,0xEEE]
 split=[0,0x200,0x222,0x264,0x486,0x6A8,0x642,0xA86,0xECA,0xA62,0x444,0x888,0x46A,0x8AE,0xCCC,0xEEE]
 # Reflection colors are deliberately precomputed; only actor colors 4/5
 # optionally change at HBlank. No second full-bank transfer is required.
 wet=[0x200,0x200,0x422,0x644,0x642,0x622,0xA64,0xC86,0xECA,0xECA,0xA64,0xE84,0x844,0xCA8,0xA66,0xEEE]
 common=[[0x200,0xEEE,0x222,0x622,0xA64,0xECA]+[0]*10,None,wet,[0,0xEEE,0x222,0,0x4CE,0x28E,0xE84,0xA42]+[0]*8]
 for demo,pal in [('water',dry),('split',split)]:
  palettes=[common[0],pal,common[2],common[3]]
  out[demo+'_palettes.bin']=struct.pack('>64H',*(c for p in palettes for c in p))
 return out

def generate():
 for name,data in build().items():(ROOT/'res'/name).write_bytes(data)
if __name__=='__main__':generate();print('Native water/reflection, split world and shared raster atlas generated')

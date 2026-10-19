#!/usr/bin/env python3
"""Native pixel geometry and shared collision/render layouts; no resizing."""
import json
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
def vdp(r,g,b):
 if any(type(c)!=int or c<0 or c>7 for c in (r,g,b)):raise ValueError('VDP channels need 0..7')
 return (b<<9)|(g<<5)|(r<<1)
def canvas(w,h):return [[0]*w for _ in range(h)]
def sprite(p):
 out=bytearray()
 for tx in range(0,len(p[0]),8):
  for ty in range(0,len(p),8):
   for y in range(8):
    for x in range(0,8,2):out.append((p[ty+y][tx+x]<<4)|p[ty+y][tx+x+1])
 return bytes(out)
def orb(color):
 p=canvas(8,8)
 for y in range(1,7):
  for x in range(1,7):
   d=(x*2-7)**2+(y*2-7)**2
   if d<=42:p[y][x]=2 if d>28 else 1 if x+y<6 else color
 return p

def actor():
 p=canvas(16,16)
 for y in range(2,14):
  for x in range(2,14):
   p[y][x]=2 if x in (2,13) or y in (2,13) else 7 if y>10 else 8
 for y in range(5,8):
  for x in range(5,11):p[y][x]=1 if y==5 else 3
 return p

def cross():
 p=canvas(8,8)
 for i in range(8):
  if i<3 or i>4:p[3][i]=p[4][i]=p[i][3]=p[i][4]=1
 return p

def normal(n):
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):
   if (n==0 and (x==1+abs(y-3) or y==3 and x>=1)) or (n==1 and (x==6-abs(y-3) or y==3 and x<=6)) or (n==2 and (y==1+abs(x-3) or x==3 and y>=1)) or (n==3 and (y==6-abs(x-3) or x==3 and y<=6)):p[y][x]=6
 return p

def layouts():
 data=json.loads((ROOT/'res/layouts.json').read_text())
 if data.get('version')!=1 or len(data.get('layouts',[]))!=4:raise ValueError('Expected four bounded layouts')
 for area in data['layouts']:
  walls=area.get('walls',[])
  if not 1<=len(walls)<=32:raise ValueError('Wall count outside 1..32')
  for box in walls:
   if len(box)!=4 or any(type(v)!=int for v in box):raise ValueError('Invalid wall')
   x,y,w,h=box
   if not(0<=x<320 and 32<=y<224 and 1<=w<=256 and 1<=h<=192 and x+w<=320 and y+h<=224):raise ValueError('Wall outside the local view')
 return data['layouts']
def pixel(area,x,y):
 color=4 if (x%16==0 or y%16==0) else 3
 for bx,by,w,h in area['walls']:
  if bx<=x<bx+w and by<=y<by+h:
   return 8 if x in (bx,bx+w-1) or y in (by,by+h-1) else 6
 return color

def build():
 areas=layouts();tiles=[];lookup={};maps=bytearray()
 for area in areas:
  for ty in range(4,28):
   for tx in range(40):
    p=[[pixel(area,tx*8+x,ty*8+y) for x in range(8)] for y in range(8)]
    data=sprite(p)
    if data not in lookup:lookup[data]=len(tiles);tiles.append(data)
    maps.extend(struct.pack('>H',lookup[data]))
 if len(tiles)>256:raise ValueError('Pattern budget exceeded')
 actors=sprite(orb(13))+sprite(orb(8))+sprite(actor())+sprite(cross())
 actors+=b''.join(sprite(normal(i)) for i in range(4))+sprite(orb(6))
 dot=canvas(8,8);dot[3][3]=dot[3][4]=dot[4][3]=dot[4][4]=8;actors+=sprite(dot)
 palettes=[[vdp(0,0,0),vdp(7,7,7),vdp(1,1,2),vdp(6,5,3)]+[0]*12,
 [vdp(*c) for c in [(0,0,0),(7,7,7),(1,1,2),(1,2,4),(3,2,1),(5,3,1),(7,5,3),(1,3,6),(4,6,7),(5,6,7),(2,1,1),(3,4,5),(3,3,7),(7,2,4),(2,7,3),(6,7,7)]],
 [vdp(*c) for c in [(0,0,0),(7,7,7),(1,1,2),(1,1,2),(1,2,3),(2,3,4),(3,4,5),(4,5,6),(4,6,7)]]+[0]*7,
 [vdp(0,0,0),vdp(7,7,7),vdp(1,1,2)]+[0]*13]
 return {'actors.bin':actors,'ground.bin':b''.join(tiles),'maps.bin':bytes(maps),'palettes.bin':struct.pack('>64H',*(c for p in palettes for c in p))},areas,len(tiles)
def generate():
 data,areas,count=build()
 for name,blob in data.items():(ROOT/'res'/name).write_bytes(blob)
 (ROOT/'build').mkdir(exist_ok=True)
 header=['/* Generated from res/layouts.json; pixel art and collision share this source. */',f'#define CD_PATTERN_COUNT {count}']
 for i,area in enumerate(areas):
  values=','.join('{'+','.join(str(n*16) for n in box)+'}' for box in area['walls'])
  header.append(f'static CcBox const cd_layout_{i}[]={{'+values+'};')
 (ROOT/'build/layouts.h').write_text('\n'.join(header)+'\n')
 (ROOT/'res/art.json').write_text(json.dumps({'patterns':count,'sprite_patterns':13,'maps':4,'map_words':960},indent=2)+'\n')
 return count
if __name__=='__main__':print('Native Collision Lab art / shared layouts:',generate(),'background patterns')

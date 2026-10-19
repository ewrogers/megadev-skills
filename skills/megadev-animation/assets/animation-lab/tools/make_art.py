#!/usr/bin/env python3
"""Original 16x24 duelist and dummy, native sword arcs and exact hitbox guides."""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
def canvas(w,h):return [[0]*w for _ in range(h)]
def rect(p,x,y,w,h,c):
 for yy in range(max(0,y),min(len(p),y+h)):
  for xx in range(max(0,x),min(len(p[0]),x+w)):p[yy][xx]=c

def tile(p):return bytes((p[y][x]<<4)|p[y][x+1] for y in range(8) for x in range(0,8,2))
def sprite(p):
 return b''.join(tile([row[x:x+8] for row in p[y:y+8]]) for x in range(0,len(p[0]),8) for y in range(0,len(p),8))
def hero(pose):
 p=canvas(16,24)
 if pose==6:
  rect(p,1,19,14,4,2);rect(p,2,19,9,2,8);rect(p,11,19,3,2,6);return p
 offset=4 if pose==5 else 0
 rect(p,4,2+offset,8,8,2);rect(p,5,3+offset,6,6,4)
 rect(p,5,1+offset,7,4,3);rect(p,4,2+offset,8,1,8)
 rect(p,6,5+offset,7,3,6);p[6+offset][11]=2
 rect(p,4,10+offset,8,8,2);rect(p,5,10+offset,6,5,7);rect(p,6,10+offset,4,2,8)
 rect(p,5,16+offset,6,2,9)
 if pose==3:rect(p,10,10,5,3,5);rect(p,13,11,2,2,6)
 else:rect(p,3,11+offset,2,5,3);rect(p,11,11+offset,2,5,7)
 if pose==4:rect(p,3,11,9,1,13)
 if pose==5:rect(p,4,21,9,2,3)
 else:
  rect(p,5,18,2,3,3);rect(p,9,18,2,3,3)
  rect(p,4,20+(pose==1),3,2,9);rect(p,9,20+(pose==2),3,2,9)
 return p

def line(p,x0,y0,x1,y1,c,r=0):
 dx=abs(x1-x0);sx=1 if x0<x1 else -1;dy=-abs(y1-y0);sy=1 if y0<y1 else -1;err=dx+dy
 while True:
  rect(p,x0-r,y0-r,r*2+1,r*2+1,c)
  if x0==x1 and y0==y1:break
  e=err*2
  if e>=dy:err+=dy;x0+=sx
  if e<=dx:err+=dx;y0+=sy

def sword(n):
 p=canvas(32,32);tips=[(24,2),(30,16),(22,30)]
 if n:
  a=tips[n-1];b=tips[n];line(p,*a,*b,5,1);line(p,a[0]-2,a[1],b[0]-2,b[1],8)
 line(p,3,14,*tips[n],2,1);line(p,4,14,*tips[n],1)
 rect(p,1,12,6,2,6);rect(p,0,14,4,3,5);return p

def dummy(n):
 p=canvas(16,24);shift=1 if n==1 else -1 if n==2 else 0
 rect(p,7,15,2,7,4);rect(p,3,22,10,2,2);rect(p,4,21,8,1,5)
 rect(p,3+shift,3,10,14,2);rect(p,4+shift,4,8,12,4)
 rect(p,5+shift,5,6,10,5);rect(p,6+shift,8,4,4,13 if not n else 1)
 rect(p,7+shift,6,2,8,6);rect(p,5+shift,9,6,2,6);return p

def box(w,h,c):
 p=canvas(32,32)
 for y in range(h):
  for x in range(w):
   if x==0 or x==w-1 or y==0 or y==h-1:p[y][x]=c
 return p

def spark(n):
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):
   if (x==3 or y==3) and abs(x-3)+abs(y-3)<4-n:p[y][x]=1 if abs(x-3)+abs(y-3)<2 else 6
 return p

def ground():
 out=[]
 for n in range(10):
  p=[[3]*8 for _ in range(8)]
  if n<2:
   if n:p[5][3]=p[5][4]=4
  elif n<6:
   q=n-2;p=[[6 if y==0 and q<2 else 2 if y==7 else 5 if y<3 else 4 for x in range(8)] for y in range(8)]
   if q&1:
    for y in range(8):p[y][7]=2
  elif n<8:
   p=[[4]*8 for _ in range(8)]
   for y in range(8):p[y][0]=3
   for x in range(8):p[0][x]=5
  else:
   for y in range(8):p[y][3]=p[y][4]=5
  out.append(tile(p))
 return b''.join(out)

def vdp(r,g,b):
 if any(not isinstance(c,int) or c<0 or c>7 for c in (r,g,b)):raise ValueError("VDP channels need integers 0..7")
 return (b<<9)|(g<<5)|(r<<1)

def build():
 actors=b''.join(sprite(hero(n)) for n in range(7))
 actors+=b''.join(sprite(sword(n)) for n in range(3))
 actors+=b''.join(sprite(dummy(n)) for n in range(3))
 actors+=b''.join(sprite(box(w,h,c)) for w,h,c in [(20,14,13),(26,14,13),(24,14,13),(10,16,8),(14,20,6)])
 actors+=b''.join(sprite(spark(n)) for n in range(2))
 # Semantic RGB triples avoid mistaking the VDP's BGR word order for RGB.
 colors=[(0,0,0),(7,7,7),(1,1,2),(1,2,4),(3,2,1),(5,3,1),(7,5,3),
         (1,3,6),(4,6,7),(5,6,7),(2,1,1),(3,4,5),(3,3,7),(7,2,2),(2,7,3),(6,7,7)]
 palettes=[[vdp(0,0,0),vdp(7,7,7),vdp(1,1,2),vdp(6,5,3)]+[0]*12,
           [vdp(*c) for c in colors],
           [vdp(*c) for c in [(0,0,0),(7,7,7),(1,1,2),(1,2,3),(2,3,4),(3,4,5),(4,5,6)]]+[0]*9,
           [vdp(*c) for c in [(0,0,0),(7,7,7),(1,1,2),(2,1,3),(3,2,4),(4,3,5),(5,4,6)]]+[0]*9]
 return {'actors.bin':actors,'ground.bin':ground(),'palettes.bin':struct.pack('>64H',*(c for p in palettes for c in p))}
if __name__=='__main__':
 for n,data in build().items():(ROOT/'res'/n).write_bytes(data)
 print('Native art: seven 16x24 body poses, three 32x32 sword arcs, dummy poses and exact hitbox guides')

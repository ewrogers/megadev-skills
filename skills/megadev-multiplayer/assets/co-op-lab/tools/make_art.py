#!/usr/bin/env python3
"""Original native-pixel sprites and seamless 8px ground patterns; no resampling."""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
class Canvas:
 def __init__(self,w,h,c=0):self.w=w;self.h=h;self.p=[[c]*w for _ in range(h)]
 def dot(self,x,y,c):
  if 0<=x<self.w and 0<=y<self.h:self.p[y][x]=c
 def box(self,x,y,w,h,c):
  for j in range(y,y+h):
   for i in range(x,x+w):self.dot(i,j,c)
 def line(self,x,y,xx,yy,c):
  n=max(abs(xx-x),abs(yy-y),1)
  for i in range(n+1):self.dot(x+(xx-x)*i//n,y+(yy-y)*i//n,c)
 def ring(self,x,y,r,c):
  for yy in range(y-r,y+r+1):
   for xx in range(x-r,x+r+1):
    d=(xx-x)**2+(yy-y)**2
    if (r-1)**2<=d<=r*r:self.dot(xx,yy,c)
 def tiles(self):
  out=bytearray()
  for tx in range(0,self.w,8):
   for ty in range(0,self.h,8):
    for y in range(8):
     for x in range(0,8,2):out.append(self.p[ty+y][tx+x]*16+self.p[ty+y][tx+x+1])
  return bytes(out)
def build():
 frames=[]
 for face in range(4):
  for phase in range(2):
   c=Canvas(16,24);c.box(3,21,10,2,2);c.box(3,12,10,8,3);c.box(4,13,8,6,4);c.box(5,13,5,4,5)
   c.box(4,20,3,2+phase,7);c.box(10,20,3,3-phase,7)
   c.box(4,3,9,10,3);c.box(5,4,7,8,6);c.box(5,3,7,2,5);c.box(6,1,5,2,5)
   if face!=1:
    c.box(5,7,7,5,8);c.dot(6 if face!=3 else 10,8,3)
    if face==0:c.dot(10,8,3)
   else:c.box(5,7,7,5,4);c.box(6,8,5,3,5)
   if face==2:c.box(2,13,2,4,8)
   elif face==3:c.box(13,13,2,4,8)
   else:c.box(2,13,2,4,8);c.box(13,13,2,4,8)
   c.box(6,13,4,2,9);frames.append(c)
 for phase in range(2):
  c=Canvas(16,16);c.box(1,4+phase,5,7,3);c.box(10,4+phase,5,7,3);c.box(2,5+phase,3,5,10);c.box(11,5+phase,3,5,10)
  c.box(6,4,4,10,3);c.box(7,5,2,7,5);c.dot(6,3,1);c.dot(9,3,1);frames.append(c)
 for phase in range(2):
  c=Canvas(8,8);c.box(2,0,4,8,3);c.box(1,1,6,6,3);c.box(2,1,4,6,9);c.box(3,2,2,4,1);c.dot(2+phase,1,1);frames.append(c)
 for phase in range(2):
  c=Canvas(32,32);c.ring(16,16,12+phase*3,1);c.ring(16,16,10+phase*3,10)
  for x,y in [(3,4),(27,7),(7,26),(27,27)]:c.line(x-2,y,x+2,y,1);c.line(x,y-2,x,y+2,1)
  frames.append(c)
 for phase in range(2):
  c=Canvas(8,8);c.line(1,3,7,3,1);c.line(4,0,4,7,1);c.dot(3+phase,2,9);frames.append(c)
 actors=b''.join(c.tiles() for c in frames)
 ground=[]
 for i in range(16):
  c=Canvas(8,8,11)
  if i<2:
   c.box(0,0,8,1,12);c.box(0,0,1,8,12);c.dot(5,5,12)
   if i:c.box(3,2,2,1,13);c.dot(4,1,14)
  elif i<6:
   c=Canvas(8,8,3);c.box(1,1,6,6,12);c.box(1,1,6,1,14);c.box(1,2,1,5,13);c.box(2,3,4,3,11)
  elif i<10:
   c=Canvas(8,8,13);c.box(0,0,8,1,14);c.box(0,7,8,1,12);c.dot(4,3,14)
  else:
   c=Canvas(8,8,2)
   if i==10:c.box(1,1,6,6,1)
   if i==11:c.box(1,1,6,6,4)
   if i==12:c.box(1,1,6,6,3)
   if i==13:c.box(0,0,8,1,1);c.box(0,7,8,1,3)
   if i==14:c.box(0,0,8,8,2)
   if i==15:c.box(1,1,6,6,9)
  ground.append(c.tiles())
 # Shared neutral shades and gold; only cloak accents vary. UI font is index 1.
 base=[0,0xeee,0x222,0x202,0x880,0xcc6,0xeea,0x424,0x8ce,0x6ce,0xcee,0x442,0x664,0x886,0xaaa,0xeee]
 pals=[]
 for a,b,d in [(0x6a8,0x8ca,0xcee),(0x68c,0x8ae,0xcee),(0x66c,0x88e,0xccE),(0x48c,0x6ae,0xace)]:
  p=base.copy();p[4:7]=[a,b,d];pals.extend(p)
 return {'actors.bin':actors,'ground.bin':b''.join(ground),'palettes.bin':struct.pack('>64H',*pals)}
if __name__=='__main__':
 for n,d in build().items():(ROOT/'res'/n).write_bytes(d)

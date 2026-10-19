#!/usr/bin/env python3
"""Original native-resolution pixel art. No resizing, quantizer or external tools."""
from pathlib import Path
import struct
ROOT=Path(__file__).resolve().parents[1]
COLORS=[0x000,0xEEE,0x222,0x422,0x864,0x28A,0x6CE,0x8A2,0xCE6,0x28E,0x26A,0x68E,0x64A,0x8CE,0x4A8,0xACE]
def tile(p):
 return bytes((p[y][x]<<4)|p[y][x+1] for y in range(8) for x in range(0,8,2))
def sprite(p):
 h=len(p);w=len(p[0]);return b''.join(tile([r[x:x+8] for r in p[y:y+8]]) for x in range(0,w,8) for y in range(0,h,8))
def canvas(w,h):return [[0]*w for _ in range(h)]
def rect(p,x,y,w,h,c):
 for yy in range(y,y+h):
  for xx in range(x,x+w):p[yy][xx]=c

def actor(face,frame):
 p=canvas(16,24)
 rect(p,3,19,10,2,3)
 rect(p,4,3,8,8,2);rect(p,5,2,6,1,2);rect(p,5,3,6,6,5)
 rect(p,4,2,8,3,7);rect(p,6,1,5,2,8);rect(p,3,5,10,1,7)
 if face!=1:
  rect(p,6 if face==3 else 5,6,5,3,6)
  rect(p,10 if face==3 else 6,6,1,1,2)
  if face==0:p[6][9]=2
 else:rect(p,5,6,6,3,4)
 rect(p,4,10,8,9,2);rect(p,5,10,6,7,7);rect(p,6,10,3,5,8)
 rect(p,5,17,6,1,5);rect(p,3,11,2,5,4);rect(p,11,11,2,5,4)
 rect(p,3,15,2,2,6);rect(p,11,15,2,2,6)
 rect(p,5,18,2,3,3);rect(p,9,18,2,3,3)
 rect(p,4,20+(frame&1),3,2,5);rect(p,9,21-(frame&1),3,2,5)
 if face==2:p=[list(reversed(r)) for r in p]
 return p

def bug(frame):
 p=canvas(16,16)
 for y in range(3,13):
  for x in range(2,14):
   dx=x-7;dy=y-8
   if dx*dx+dy*dy<38:p[y][x]=2 if dx*dx+dy*dy>25 else 12 if x<8 else 11
 rect(p,5,5,2,2,1);rect(p,10,5,2,2,1);p[6][6]=p[6][11]=2
 for x in [3,12]:rect(p,x,12,1,2,5)
 p[2][4+frame]=p[2][11-frame]=13
 return p

def gem(frame):
 p=canvas(8,8)
 for y in range(7):
  for x in range(8):
   d=abs(x-3)+abs(y-3)
   if d<=3:p[y][x]=2 if d==3 else 1 if x+y<5+frame else 6 if x<4 else 5
 return p

def pulse(frame):
 p=canvas(32,32)
 for y in range(32):
  for x in range(32):
   d=(x-15)*(x-15)+(y-15)*(y-15)
   if 180<=d<=225 and ((x+y+frame)&3)!=0:p[y][x]=1 if (x+y)&1 else 8
 return p

def spark(frame):
 p=canvas(8,8)
 for y in range(8):
  for x in range(8):
   if (x==3 or y==3) and abs(x-3)+abs(y-3)<4-frame:p[y][x]=1 if abs(x-3)+abs(y-3)<2 else 6
 return p

def ground():
 out=[]
 # 0 floor, 1 alternate floor; 2..5 wall quadrants; 6/7 path; 8/9 gate.
 for n in range(10):
  p=[[3]*8 for _ in range(8)]
  if n<2:
   p=[[3]*8 for _ in range(8)]
   if n:p[5][2]=p[4][3]=4
  elif n<6:
   q=n-2;p=[[6 if y==0 and q<2 else 2 if (x==7 and q&1) or y==7 else 5 if y<3 else 4 for x in range(8)] for y in range(8)]
   if q&1:p[4][6]=3
  elif n<8:
   p=[[4]*8 for _ in range(8)]
   for y in range(8):p[y][0]=3
   for x in range(8):p[0][x]=5
  else:
   p=[[3]*8 for _ in range(8)]
   for y in range(8):
    for x in range(8):
     if abs(x-(2 if n==8 else 5))==abs(y-3)//2:p[y][x]=6
  out.append(tile(p))
 return b''.join(out)

def build():
 actors=b''.join(sprite(actor(f,n)) for f in range(4) for n in range(2))
 actors+=b''.join(sprite(bug(n)) for n in range(2))
 actors+=b''.join(sprite(gem(n)) for n in range(2))
 actors+=b''.join(sprite(pulse(n)) for n in range(2))
 actors+=b''.join(sprite(spark(n)) for n in range(2))
 palettes=[[0,0xEEE,0x422,0x6CE]+[0]*12,COLORS,
 [0,0xEEE,0x222,0x242,0x464,0x686,0x8C8]+[0]*9,
 [0,0xEEE,0x222,0x224,0x446,0x668,0x8AC]+[0]*9]
 return {'actors.bin':actors,'ground.bin':ground(),'palettes.bin':struct.pack('>64H',*(c for pal in palettes for c in pal))}
if __name__=='__main__':
 for n,b in build().items():(ROOT/'res'/n).write_bytes(b)
 print('Native art: 16x24 hero, 16x16 beetles, 8x8 gems, 32x32 pulse; four legal VDP palettes')

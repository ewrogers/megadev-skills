#!/usr/bin/env python3
"""Original native pixel art for an illustrative moonlit clockwork courtyard."""
from pathlib import Path
import json,struct
ROOT=Path(__file__).resolve().parents[1]
def canvas(w,h):return [[0]*w for _ in range(h)]
def rect(p,x,y,w,h,c):
 for yy in range(max(0,y),min(len(p),y+h)):
  for xx in range(max(0,x),min(len(p[0]),x+w)):p[yy][xx]=c
def tile(p):return bytes(p[y][x]*16+p[y][x+1] for y in range(8) for x in range(0,8,2))
def pack(p,sprite=True):
 h,w=len(p),len(p[0]);xy=[(x,y) for x in range(0,w,8) for y in range(0,h,8)] if sprite else [(x,y) for y in range(0,h,8) for x in range(0,w,8)]
 return b''.join(tile([row[x:x+8] for row in p[y:y+8]]) for x,y in xy)
def hero(face,walk):
 p=canvas(16,24);rect(p,3,21,10,2,2)
 rect(p,4,2,8,9,2);rect(p,5,1,6,2,2);rect(p,5,3,6,7,12)
 rect(p,4,2,8,4,3);rect(p,5,2,6,2,5);rect(p,3,5,10,1,2)
 if face!=1:
  rect(p,6 if face==3 else 5,6,5,3,13);rect(p,9 if face==3 else 6,6,1,2,2)
  if face==0:rect(p,9,6,1,2,2)
 else:rect(p,5,6,6,3,3)
 rect(p,4,11,8,9,2);rect(p,5,11,6,7,4);rect(p,6,11,2,6,6)
 rect(p,3,12,2,5,3);rect(p,11,12,2,5,3);rect(p,3,16,2,2,13);rect(p,11,16,2,2,13)
 rect(p,5,18,6,1,10);rect(p,5,19,2,3,3);rect(p,9,19,2,3,3)
 rect(p,4,21+walk,3,2,2);rect(p,9,22-walk,3,2,2)
 return [list(reversed(row)) for row in p] if face==2 else p
def enemy(kind,face,walk):
 p=canvas(16,16);dark,mid,light=[(3,4,5),(6,7,8),(9,10,11)][kind]
 rect(p,3,12,10,3,2);rect(p,2,4,12,8,1);rect(p,3,3,10,9,dark);rect(p,4,2,8,9,mid);rect(p,5,2,6,2,light)
 if kind==0:rect(p,5,0,6,2,dark);rect(p,6,0,4,1,light)
 elif kind==1:rect(p,1,6,2,5,dark);rect(p,13,6,2,5,dark)
 else:rect(p,0,5,4,2,light);rect(p,12,5,4,2,light)
 if face!=1:
  x=8 if face==3 else 3;rect(p,x,5,5,4,1);rect(p,x+1,6,3,2,15)
  if face==0:rect(p,10,5,3,4,1);rect(p,11,6,1,2,15)
 else:rect(p,5,5,6,4,dark);rect(p,7,6,2,1,light)
 rect(p,3,12+(walk&1),3,2,mid);rect(p,10,13-(walk&1),3,2,mid)
 if face==2:p=[list(reversed(row)) for row in p]
 return p
def glyph(rows,color=1):
 p=canvas(8,8)
 for y,row in enumerate(rows):
  for x,c in enumerate(row):
   if c!=' ':p[y][x]=color
 return p
def pulse(frame):
 p=canvas(32,32);radius=10+frame*4
 for y in range(32):
  for x in range(32):
   d=(x-15)**2+(y-15)**2
   if (radius-1)**2<=d<=radius**2 and ((x+y+frame)&3)!=0:p[y][x]=1 if (x+y)&1 else 6
 return p
def ground(kind):
 p=[[3]*16 for _ in range(16)]
 if kind<2:
  rect(p,0,0,16,1,4);rect(p,0,1,1,15,2);rect(p,15,1,1,15,2);rect(p,1,15,14,1,2)
  if kind:rect(p,5,6,3,1,4);rect(p,8,10,2,1,2)
 elif kind==2:
  p=[[1]*16 for _ in range(16)];rect(p,1,1,14,13,2);rect(p,1,1,14,2,6);rect(p,2,3,12,3,5);rect(p,2,7,12,6,3);rect(p,3,7,10,1,4)
  rect(p,1,14,14,1,1);rect(p,7,8,1,5,2)
 elif kind==3:
  p=ground(0);rect(p,3,3,10,10,2);rect(p,4,4,8,8,8);rect(p,6,4,4,8,9);rect(p,4,6,8,4,9);rect(p,7,6,2,4,10)
 elif kind==4:
  p=ground(0)
  for y in range(3,13):
   for x in range(3,13):
    if abs(x-7)+abs(y-7)<=5:p[y][x]=14 if x==7 or y==7 else 12
 elif kind==5:
  p=ground(0);rect(p,4,9,8,4,12);rect(p,5,10,6,3,13)
  for x,y in [(4,5),(7,3),(10,5),(6,6),(9,7)]:rect(p,x,y,3,4,9);rect(p,x,y,2,1,10)
 return p
def build():
 actors=bytearray();meta={}
 def add(name,images):
  meta[name]=len(actors)//32
  for p in images:actors.extend(pack(p))
 add('HERO',[hero(f,n) for f in range(4) for n in range(2)])
 add('ENEMY',[enemy(k,f,n) for k in range(3) for f in range(4) for n in range(2)])
 add('ALERT',[glyph(['   ##','   ##','   ##','   ##','','   ##'],1)])
 add('QUESTION',[glyph(['  ####','     ##','    ##','   ##','','   ##'],10)])
 z=canvas(16,8)
 for yy,row in enumerate(['####      ###','  #        #',' #        #','####     ###']):
  for xx,c in enumerate(row):
   if c=='#':z[yy+1][xx+1]=1
 add('SLEEP',[z])
 gems=[]
 for n in range(2):
  p=canvas(8,8)
  for y in range(8):
   for x in range(8):
    d=abs(x-3)+abs(y-3)
    if d<=3:p[y][x]=2 if d==3 else 1 if x+y<5+n else 6 if x<4 else 5
  gems.append(p)
 add('LENS',gems)
 add('BOLT',[glyph(['','   ##','  ####',' ######','  ####','   ##'],14)])
 add('DOT',[glyph(['','','   ##','   ##'],1)])
 add('HEART',[glyph(['',' ## ##','#######','#######',' #####','  ###','   #'],14)])
 add('EMPTY',[glyph(['',' ## ##','#  #  #','#     #',' #   #','  # #','   #'],3)])
 add('BAR',[glyph(['########']*6,1),glyph(['########','#      #','#      #','#      #','#      #','########'],3)])
 add('PULSE',[pulse(0),pulse(1)])
 meta['COUNT']=len(actors)//32
 palettes=[
  [0x000,0xEEE,0x200,0x446,0x68A,0xACE,0x8CE,0x0EE,0x024,0x068,0x0AC,0x2CE,0x44A,0x88E,0x46E,0xEEE],
  [0x000,0xEEE,0x222,0x422,0xA64,0xEA8,0xECA,0x688,0xACA,0x2A8,0x2CE,0x6EE,0x68A,0xACE,0x66E,0xEEE],
  [0x000,0x222,0x244,0x466,0x688,0x8AA,0xACC,0xCEE,0x242,0x464,0x686,0x8A8,0x246,0x68A,0xACE,0xEEE],
  [0x000,0x222,0x422,0x626,0xA6A,0xEAE,0x246,0x28A,0x6CE,0x442,0x884,0xCC8,0x68A,0xACE,0x46E,0xEEE]
 ]
 header='#ifndef ENEMY_ART_META_H\n#define ENEMY_ART_META_H\n'+''.join(f'#define ART_{k} {v}u\n' for k,v in meta.items())+'#define GROUND_COUNT 24u\n#endif\n'
 return {'actors.bin':bytes(actors),'ground.bin':b''.join(pack(ground(k),False) for k in range(6)),'palettes.bin':struct.pack('>64H',*(v for pal in palettes for v in pal))},header
def generate(root=ROOT):
 root=Path(root);files,header=build()
 for n,b in files.items():(root/'res'/n).write_bytes(b)
 (root/'src/art_meta.h').write_text(header)
 (root/'res/art-report.json').write_text(json.dumps({'native_pixels':True,'sprite_tiles':len(files['actors.bin'])//32,'ground_tiles':24,'palettes':4,'source':'Original procedural Lab artwork; illustrative style'},indent=2)+'\n')
if __name__=='__main__':generate()

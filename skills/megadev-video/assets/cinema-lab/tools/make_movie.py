#!/usr/bin/env python3
"""Original native-resolution Lantern Flight animation. No resampling/AI assets.

The fixed CIN1 example uses 8 packed VDP frames per 31-sector file, a one-sector
header and per-frame sum16. This is a teaching format, not an SDK video codec.
"""
from pathlib import Path
import json,math,struct
W,H,N=160,96,48
PALETTE=[(0,0,0),(0,0,34),(34,34,68),(68,34,102),(68,68,136),(102,102,170),(170,136,204),(238,204,238),(34,68,68),(34,102,102),(68,136,136),(136,170,136),(170,102,68),(204,136,68),(238,204,102),(238,238,204)]
def frame(n):
 p=[[1]*W for _ in range(H)]
 def dot(x,y,c):
  if 0<=x<W and 0<=y<H:p[y][x]=c
 def rect(x,y,w,h,c):
  for yy in range(max(0,y),min(H,y+h)):
   for xx in range(max(0,x),min(W,x+w)):p[yy][xx]=c
 def line(x0,y0,x1,y1,c):
  dx=abs(x1-x0);sx=1 if x0<x1 else -1;dy=-abs(y1-y0);sy=1 if y0<y1 else -1;err=dx+dy
  while True:
   dot(x0,y0,c)
   if x0==x1 and y0==y1:break
   e=2*err
   if e>=dy:err+=dy;x0+=sx
   if e<=dx:err+=dx;y0+=sy
 def ellipse(cx,cy,rx,ry,c):
  for y in range(cy-ry,cy+ry+1):
   for x in range(cx-rx,cx+rx+1):
    if (x-cx)**2*ry**2+(y-cy)**2*rx**2<=rx**2*ry**2:dot(x,y,c)
 # Modest ordered transition bands, not blanket texture on every asset.
 for y in range(58):
  for x in range(W):
   band=min(3,y//16);p[y][x]=1+band
   if y%16>12 and (x+y)%2==0:p[y][x]=min(4,2+band)
 for i in range(43):
  x=(i*47-n//8)%W;y=(i*i*13)%40
  dot(x,y,7 if (i+n//6)%5 else 15)
 ellipse(124,23,15,15,6);ellipse(127,20,14,14,7)
 ellipse(130,16,4,3,6);ellipse(119,27,3,2,5)
 # Far clouds drift independently of the foreground skyline.
 for i in range(5):
  x=(i*51-n//3)%210-25;y=38+(i%2)*6
  ellipse(x,y,25,3,4);ellipse(x-6,y-2,11,4,4);line(x-20,y+2,x+20,y+2,5)
 rect(0,66,W,30,8)
 for y in range(68,96,4):
  for x in range((y*7-n*2)%17,W,19):rect(x,y,9,1,9 if y<85 else 10)
 # Backdrop towers, moving twice as fast as distant clouds.
 for i in range(10):
  x=(i*23-n)%230-30;y=49+(i%3)*5
  rect(x,y,15,22,2);ellipse(x+7,y,7,3,3);rect(x+7,y-8,1,6,2)
  for xx in range(x+3,x+14,5):rect(xx,y+5,2,3,13 if i%2 else 6)
 # Near lighthouse: warm windows and cool stone against the water.
 x=147-n*2
 rect(x,51,13,35,2);rect(x+2,53,9,31,5);rect(x+5,58,3,7,14)
 rect(x-2,49,17,3,12);rect(x+1,41,11,8,2);rect(x+3,42,7,6,15)
 for yy in range(36,42):rect(x+6-(yy-36),yy,(yy-36)*2+1,1,3)
 for yy in range(87,95,3):rect(x-5,yy,23,1,10)
 # Airship floats toward the beacon, with native outlines and a warm lamp.
 cx=32+n*2;cy=31+(0,1,2,2,3,3,3,3,3,2,2,1,0,-1,-2,-2,-3,-3,-3,-3,-3,-2,-2,-1)[n%24]
 ellipse(cx,cy,17,8,2);ellipse(cx,cy-1,15,6,12);ellipse(cx-2,cy-3,11,3,14)
 for xx in [cx-9,cx,cx+9]:line(xx,cy-6,xx,cy+5,13)
 line(cx-8,cy+6,cx-5,cy+13,2);line(cx+8,cy+6,cx+5,cy+13,2)
 rect(cx-7,cy+12,15,5,2);rect(cx-5,cy+12,10,3,13);rect(cx-10,cy+13,2,3,14)
 line(cx+7,cy+12,cx+10,cy+9,2);dot(cx+10,cy+9,15)
 # Lantern particles, intentionally low count and crisp.
 for i in range(6):dot((cx-22-i*7)%W,cy+6+(i*7+n//3)%18,14 if i%2 else 6)
 out=bytearray()
 for ty in range(H//8):
  for tx in range(W//8):
   for y in range(8):
    for x in range(0,8,2):out.append(p[ty*8+y][tx*8+x]<<4|p[ty*8+y][tx*8+x+1])
 return bytes(out)
def pcm(v):
 n=max(-127,min(127,round(v)));return n if n>0 else 128|(-n) if n<0 else 128

def generate(root):
 root=Path(root);(root/'disc').mkdir(exist_ok=True);(root/'res').mkdir(exist_ok=True)
 frames=[frame(n) for n in range(N)];assert all(len(f)==7680 for f in frames)
 for first in range(0,N,8):
  header=bytearray(2048);header[:4]=b'CIN1';struct.pack_into('>8H',header,4,1,W,H,first,8,7680,30,N)
  struct.pack_into('>8H',header,32,*(sum(struct.unpack(">3840H",f))&65535 for f in frames[first:first+8]))
  data=header+b''.join(frames[first:first+8]);assert len(data)==63488
  (root/'disc'/f'C{first//8:02}.CIN').write_bytes(data)
 def md(c):r,g,b=c;return (b//34<<9)|(g//34<<5)|(r//34<<1)
 (root/'res/palette.bin').write_bytes(b''.join(struct.pack('>H',md(c)) for c in PALETTE))
 (root/'res/poster.bin').write_bytes(frames[0])
 # Four-second original ambient phrase; attack/release envelopes keep seams quiet.
 chords=[(220,330,440),(174,261,349),(196,294,392),(165,247,330)]
 score=[]
 for i in range(32768):
  beat=i//8192;t=(i%8192)/8192;env=min(1,t*12,(1-t)*12)
  value=sum(15*math.sin(math.tau*f*t) for f in chords[beat])*env
  value+=12*math.exp(-t*5)*math.sin(math.tau*(660,523,588,494)[beat]*t)
  score.append(pcm(value))
 score=bytes(score)
 chime=bytes(pcm(72*math.exp(-i/560)*math.sin(math.tau*880*i/8192)) for i in range(4094))+b'\x80\xff'
 assert 255 not in score and 255 not in chime[:-1]
 (root/'res/score.pcm').write_bytes(score);(root/'res/chime.pcm').write_bytes(chime)
 (root/'res/movie-report.json').write_text(json.dumps({'width':W,'height':H,'frames':N,'nominal_images_per_second':10,'native_bytes_per_frame':7680,'chunk_bytes':63488,'chunks':6,'disc_payload_bytes':380928,'sub_staging_bytes':126976,'main_frame_bytes':7680,'upload_batch_bytes':4096,'vram_slots_bytes':15360,'palette':PALETTE},indent=2)+'\n')
if __name__=='__main__':generate(Path(__file__).resolve().parents[1])

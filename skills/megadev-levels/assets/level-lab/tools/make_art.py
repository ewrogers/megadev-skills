#!/usr/bin/env python3
"""Original 16x16 atlas and 16x24 explorer; all pixels authored at native size."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
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


def terrain(n):
 p=canvas(16,16)
 for y in range(16):
  for x in range(16):
   c=3
   if n==1 and (x,y) in [(3,5),(4,4),(11,12)]:c=4
   if n in (2,3):c=6 if x==0 or y==0 else 5 if y<3 else 4
   if n in (4,5):c=2 if x in (0,15) or y==15 else 7 if y==0 else 6 if y<3 else 5 if y<8 else 4
   if n==4 and y in (7,8):c=2 if x<8 else 6
   if n==6:c=9 if y in (3,11) and x in (2,3,4,9,10) else 8
   if n==7:c=6 if y<4 else 9 if y==5 else 8
   if n==8:c=2 if 5<=x<=10 else 0
   if n==8 and 6<=x<=9:c=5 if x<8 else 4
   if n==9:
    d=(x-7)*(x-7)+(y-8)*(y-8)
    c=2 if 45<=d<62 else 12 if d<45 and x+y<15 else 11 if d<45 else 0
   if n==10:c=3
   if n==10 and (x-7)*(x-7)+(y-7)*(y-7)<8:c=1 if (x,y)==(7,7) else 13
   if n==11:c=2 if x in (0,15) or y in (0,15) else 7 if y<3 else 6 if x+y<16 else 5
   if n==12:c=4
   if n==12 and (x==7 or x==8 or abs(x-8)==abs(y-7) and y<8):c=14
   if n==13:
    d=abs(x-7)+abs(y-7);c=2 if d==7 else 14 if d<7 else 5
    if d<4:c=1
   if n==14:c=2 if x in (0,15) or y%5==0 else 6 if y%5==1 else 5
   if n==15:c=2 if x in (3,12) and y>3 else 6 if 4<=x<=11 and y>3 else 3
   p[y][x]=c
 return p

def palettes():
 return [
 [[0,0,0],[7,7,6],[1,1,2],[2,3,4]]+[[0,0,0]]*12,
 [[0,0,0],[7,7,7],[1,1,2],[2,2,3],[2,3,4],[5,3,1],[7,5,3],[1,3,5],[3,6,7],[4,5,7],[3,2,5],[5,3,6],[6,5,7],[7,4,2],[7,6,2],[7,7,5]],
 [[0,0,0],[7,7,6],[1,2,2],[2,4,2],[3,5,3],[4,4,3],[6,6,4],[7,7,5],[1,3,4],[3,5,6],[2,3,2],[2,4,1],[4,6,2],[7,3,3],[7,6,2],[6,4,2]],
 [[0,0,0],[7,7,7],[1,1,3],[1,3,4],[2,4,5],[3,4,5],[5,6,7],[6,7,7],[1,2,5],[3,5,7],[1,2,4],[1,3,5],[3,5,6],[7,4,5],[6,6,3],[5,3,5]]]

def generate(sources=False):
 res=ROOT/'res';res.mkdir(exist_ok=True);pals=palettes()
 actors=b''.join(sprite(actor(f,n)) for f in range(4) for n in range(2))
 gem=canvas(8,8)
 for y in range(8):
  for x in range(8):
   d=abs(x-3)+abs(y-3)
   if d<=3:gem[y][x]=2 if d==3 else 1 if x+y<6 else 14
 actors+=sprite(gem)
 (res/'actors.bin').write_bytes(actors)
 if not sources:return
 from PIL import Image
 (res/'palettes.json').write_text(json.dumps(pals,indent=2)+'\n')
 im=Image.new('RGBA',(128,64));tiles=[]
 for i in range(32):
  p=terrain(i%16);tiles.append({'id':i,'properties':[{'name':'palette','type':'int','value':2+i//16},{'name':'priority','type':'bool','value':i%16==9}]})
  for y in range(16):
   for x in range(16):
    n=p[y][x];rgb=tuple(c*255//7 for c in pals[2+i//16][n]);im.putpixel(((i%8)*16+x,(i//8)*16+y),rgb+(255 if n else 0,))
 im.save(res/'terrain.png')
 ts={'type':'tileset','version':'1.10','name':'Native terrain','tilewidth':16,'tileheight':16,'tilecount':32,'columns':8,'margin':0,'spacing':0,'image':'terrain.png','imagewidth':128,'imageheight':64,'tiles':tiles}
 (res/'terrain.tsj').write_text(json.dumps(ts,indent=2)+'\n')
 for area_id,name in enumerate(['orchard','archive']):
  w,h=48,32;ground=[];fore=[0]*(w*h);solid=[0]*(w*h);offset=area_id*16
  for y in range(h):
   for x in range(w):
    n=1 if (x*3+y*7)%23==0 else 0
    if y in (15,16,17) or x in (9,10,34,35):n=2
    if x in (0,w-1) or y in (0,h-1):n=4;solid[y*w+x]=1
    if 27<=x<=36 and 4<=y<=10:n=6;solid[y*w+x]=1
    if 20<=x<=24 and 22<=y<=25:n=4;solid[y*w+x]=1
    if area_id and 3<=x<=6 and 6<=y<=11:n=6;solid[y*w+x]=1
    if (x,y) in [(16,6),(18,7),(39,22),(42,24),(13,24),(15,26)]:
     n=8;solid[y*w+x]=1
     for dy,dx in [(-1,0),(-1,-1),(0,-1)]:fore[(y+dy)*w+x+dx]=10+offset
    if n==0 and (x*7+y*11)%139==0:n=10
    if y==15 and x in (2,44):n=12
    if 15<=y<=17 and x in (1,2,3,44,45,46):n=13 if (y==16 and x in (2,45)) else 11
    ground.append(n+1+offset)
  points=[('entry',80,264),('return',680,264),('north',400,56),('southwest',80,440),('southeast',680,440)]
  for _,px,py in points:
   for y in range(py//16-1,py//16+2):
    for x in range(px//16-1,px//16+2):ground[y*w+x]=3+offset;solid[y*w+x]=0;fore[y*w+x]=0
  objects=[{'id':i+1,'name':s,'class':'Spawn','point':True,'x':x,'y':y,'width':0,'height':0,'rotation':0,'visible':True} for i,(s,x,y) in enumerate(points)]
  for i,(x,target_spawn) in enumerate([(24,'return'),(720,'entry')]):
   objects.append({'id':10+i,'name':['west','east'][i],'class':'Exit','x':x,'y':240,'width':32,'height':48,'rotation':0,'visible':True,'properties':[{'name':'target','type':'string','value':'archive' if not area_id else 'orchard'},{'name':'spawn','type':'string','value':target_spawn}]})
  layers=[{'id':i+1,'name':n,'type':'tilelayer','width':w,'height':h,'x':0,'y':0,'opacity':1,'visible':n!='Collision','data':a} for i,(n,a) in enumerate([('Ground',ground),('Foreground',fore),('Collision',[5+offset if s else 0 for s in solid])])]
  layers.append({'id':4,'name':'Objects','type':'objectgroup','x':0,'y':0,'opacity':1,'visible':True,'draworder':'topdown','objects':objects})
  m={'type':'map','version':'1.10','tiledversion':'1.11.2','orientation':'orthogonal','renderorder':'right-down','infinite':False,'width':w,'height':h,'tilewidth':16,'tileheight':16,'tilesets':[{'firstgid':1,'source':'terrain.tsj'}],'layers':layers,'nextlayerid':5,'nextobjectid':12}
  (res/(name+'.tmj')).write_text(json.dumps(m,indent=2)+'\n')
 spec={'version':1,'palette':'palettes.json','areas':[{'id':'orchard','map':'orchard.tmj'},{'id':'archive','map':'archive.tmj'}]}
 (res/'world.json').write_text(json.dumps(spec,indent=2)+'\n')
if __name__=='__main__':
 import sys
 generate('--sources' in sys.argv);print('Native explorer regenerated; authoring maps/atlas preserved unless --sources is explicit')

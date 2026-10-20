#!/usr/bin/env python3
"""Pack native procedural art for Megadev; no resampling or SGDK converter."""
from pathlib import Path
import json,struct,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(Path(__file__).resolve().parent))
import iso_art as art
def tile(c,x,y):return bytes(c.p[(y+j)*c.w+x+i]*16+c.p[(y+j)*c.w+x+i+1] for j in range(8) for i in range(0,8,2))
def part(c,y,h):return b''.join(tile(c,x,yy) for x in range(0,c.w,8) for yy in range(y,y+h,8))
def pack(data):
 # Literal/repeat packets; no dependency on a runtime SDK codec.
 out=bytearray();i=0
 while i<len(data):
  run=1
  while i+run<len(data) and data[i+run]==data[i] and run<128:run+=1
  if run>=3:out.extend((0x80|(run-1),data[i]));i+=run;continue
  start=i;i+=run
  while i<len(data) and i-start<128:
   n=1
   while i+n<len(data) and data[i+n]==data[i] and n<3:n+=1
   if n>=3:break
   i+=min(n,128-(i-start))
  out.append(i-start-1);out.extend(data[start:i])
 return bytes(out)
def build():
 sprites=bytearray();meta={}
 def add(name,frames):
  meta[name]=len(sprites)//32
  for c in frames:
   for y in range(0,c.h,32):sprites.extend(part(c,y,min(32,c.h-y)))
 add('DECK',[art.deck()]);add('POST',[art.post()]);add('RAMP_L',[art.ramp()]);add('RAMP_R',[art.ramp(True)])
 add('TOWER',[art.tower()]);add('TREE',[art.tree()]);add('FOUNTAIN',[art.fountain(i) for i in range(3)])
 add('HERO',[art.hero(i) for i in range(16)]);add('MARKER',[art.marker()]);meta['COUNT']=len(sprites)//32
 ground=art.ground();patterns=[];lookup={};mapping=[]
 for y in range(0,ground.h,8):
  for x in range(0,ground.w,8):
   p=tile(ground,x,y)
   if p not in lookup:lookup[p]=len(patterns);patterns.append(p)
   mapping.append(lookup[p])
 meta['GROUND_COUNT']=len(patterns)
 palettes=[[0,0xEEE,0x200,0x446,0x68A,0xACE,0x8CE,0x0EE,0x024,0x068,0x0AC,0x2CE,0x44A,0x88E,0x46E,0xEEE]]
 for pal in [art.WORLD,art.PROPS,art.HERO]:palettes.append([(b<<9)|(g<<5)|(r<<1) for r,g,b in pal])
 header='#ifndef ISOMETRIC_ART_META_H\n#define ISOMETRIC_ART_META_H\n'+''.join(f'#define ART_{k} {v}u\n' for k,v in meta.items())+'\n#endif\n'
 return {'actors.bin':bytes(sprites),'ground.bin':b''.join(patterns),'map.bin':struct.pack('>2048H',*mapping),'palettes.bin':struct.pack('>64H',*(v for p in palettes for v in p))},header
def generate(root=ROOT):
 files,header=build()
 files.update({n.replace('.bin','.rle'):pack(b) for n,b in list(files.items()) if n in ('actors.bin','ground.bin','map.bin')})
 for n,b in files.items():(Path(root)/'res'/n).write_bytes(b)
 (Path(root)/'src/art_meta.h').write_text(header)
 (Path(root)/'res/art-report.json').write_text(json.dumps({'native_pixels':True,'sprite_tiles':len(files['actors.bin'])//32,'ground_tiles':len(files['ground.bin'])//32,'map_tiles':[64,32],'palettes':4,'source':'Original procedural Lab art; illustrative style'},indent=2)+'\n')
if __name__=='__main__':generate()

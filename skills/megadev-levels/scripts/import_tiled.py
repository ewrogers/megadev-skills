#!/usr/bin/env python3
"""Compile a deliberately bounded Tiled JSON profile; never silently lose features."""
import argparse
import json
import hashlib
from pathlib import Path
import struct
MAX_CELLS=4096
H,V,D,R=0x80000000,0x40000000,0x20000000,0x10000000

def integer(v,lo,hi,label):
 if type(v)!=int or not lo<=v<=hi:raise ValueError(f'{label}: expected integer {lo}..{hi}')
 return v

def read(path):
 def pairs(items):
  out={}
  for k,v in items:
   if k in out:raise ValueError('Duplicate JSON key: '+k)
   out[k]=v
  return out
 return json.loads(Path(path).read_text(),object_pairs_hook=pairs)

def local(root,folder,name):
 if not isinstance(name,str):raise ValueError('Expected local relative file path')
 p=(folder/name).resolve()
 if Path(name).is_absolute() or not p.is_relative_to(root):raise ValueError('Asset path escapes project')
 return p

def props(obj,allowed):
 out={}
 for p in obj.get('properties',[]):
  name=p.get('name')
  if name not in allowed or name in out:raise ValueError('Unknown or duplicate property: '+str(name))
  out[name]=p.get('value')
 return out

def reject(obj,defaults,label):
 for key,value in defaults.items():
  if key in obj and obj[key]!=value:raise ValueError(f'{label}: unsupported {key}')

def palette(path):
 lines=read(path)
 if not isinstance(lines,list) or len(lines)!=4 or any(len(line)!=16 for line in lines):raise ValueError('Need four palettes of sixteen RGB triples')
 for line in lines:
  for rgb in line:
   if not isinstance(rgb,list) or len(rgb)!=3:raise ValueError('Expected RGB triples')
   for c in rgb:integer(c,0,7,'VDP color channel')
 return lines

def tileset(root,folder,entry,palettes):
 from PIL import Image
 if integer(entry.get('firstgid'),1,1,'first GID')!=1:raise ValueError('The shared tileset must start at GID 1')
 if 'source' in entry:
  path=local(root,folder,entry['source']);ts=read(path);folder=path.parent
 else:ts=entry
 reject(ts,{'tilewidth':16,'tileheight':16,'margin':0,'spacing':0,'tileoffset':{'x':0,'y':0}},'Tileset')
 if ts.get('tilewidth')!=16 or ts.get('tileheight')!=16:raise ValueError('Tileset must use 16x16 tiles')
 count=integer(ts.get('tilecount'),1,64,'tile count');cols=integer(ts.get('columns'),1,64,'columns')
 image=local(root,folder,ts.get('image'))
 with Image.open(image) as im:
  if im.size!=(ts.get('imagewidth'),ts.get('imageheight')) or im.width!=cols*16 or im.height*cols!=count*16:raise ValueError('Atlas dimensions disagree with tile count/grid')
  im=im.convert('RGBA')
 attrs=[(2,False)]*count;seen=set()
 for tile in ts.get('tiles',[]):
  i=integer(tile.get('id'),0,count-1,'tile id')
  if i in seen:raise ValueError('Duplicate tile metadata')
  seen.add(i)
  if any(k in tile for k in ['animation','objectgroup','image']):raise ValueError('Tile animation, objects and image collections need another profile')
  p=props(tile,{'palette','priority'});bank=integer(p.get('palette',2),2,3,'tile palette')
  priority=p.get('priority',False)
  if type(priority)!=bool:raise ValueError('Priority must be boolean')
  attrs[i]=(bank,priority)
 patterns=bytearray();words=[]
 for i,(bank,priority) in enumerate(attrs):
  colors={tuple(c*255//7 for c in rgb):n for n,rgb in reversed(list(enumerate(palettes[bank]))) if n}
  pixels=[]
  for y in range(16):
   row=[]
   for x in range(16):
    r,g,b,a=im.getpixel(((i%cols)*16+x,(i//cols)*16+y))
    if a==0:row.append(0)
    elif a!=255 or (r,g,b) not in colors:raise ValueError(f'Tile {i}: color/alpha outside palette {bank}; prepare art explicitly')
    else:row.append(colors[r,g,b])
   pixels.append(row)
  for ty,tx in [(0,0),(0,8),(8,0),(8,8)]:
   for y in range(ty,ty+8):
    for x in range(tx,tx+8,2):patterns.append((pixels[y][x]<<4)|pixels[y][x+1])
  words.append((bank<<13)|(0x8000 if priority else 0))
 return count,bytes(patterns),struct.pack('>'+str(count)+'H',*words)

def decode(gid,count):
 integer(gid,0,0xffffffff,'GID')
 if gid&D:raise ValueError('Diagonal tile transforms require pre-baked art')
 plain=gid&0x0fffffff # Clear all four flags, including the stale hex rotation flag.
 if not plain:
  if gid&(H|V):raise ValueError('Flipped empty GID')
  return 0
 if plain>count:raise ValueError('GID outside shared tileset')
 return plain|(0x4000 if gid&H else 0)|(0x8000 if gid&V else 0)

def area(root,path,count):
 m=read(path)
 reject(m,{'orientation':'orthogonal','infinite':False,'tilewidth':16,'tileheight':16,'renderorder':'right-down'},'Map')
 if m.get('tilewidth')!=16 or m.get('tileheight')!=16:raise ValueError('Map must use 16x16 tiles')
 if m.get('type')!='map' or m.get('orientation')!='orthogonal' or m.get('infinite',False):raise ValueError('Expected finite orthogonal map')
 w=integer(m.get('width'),20,96,'map width');h=integer(m.get('height'),12,64,'map height')
 if w*h>MAX_CELLS:raise ValueError('Area cell budget exceeded')
 layers={}
 for layer in m.get('layers',[]):
  name=layer.get('name')
  if name not in ('Ground','Foreground','Collision','Objects') or name in layers:raise ValueError('Unknown or duplicate layer: '+str(name))
  reject(layer,{'offsetx':0,'offsety':0,'x':0,'y':0,'opacity':1,'parallaxx':1,'parallaxy':1,'tintcolor':None,'blendmode':'normal'},name)
  if name!='Collision' and layer.get('visible',True) is not True:raise ValueError('Hidden visual/object layer')
  if name=='Objects':
   if layer.get('type')!='objectgroup':raise ValueError('Objects needs an object layer')
  else:
   if layer.get('type')!='tilelayer' or layer.get('width')!=w or layer.get('height')!=h or any(k in layer for k in ('chunks','encoding','compression')):raise ValueError('Expected full-size numeric JSON tile layer')
   data=layer.get('data')
   if not isinstance(data,list) or len(data)!=w*h:raise ValueError('Wrong layer cell count')
   layers[name]=[decode(v,count) for v in data];continue
  layers[name]=layer.get('objects',[])
 if set(layers)!=set(('Ground','Foreground','Collision','Objects')):raise ValueError('Need Ground, Foreground, Collision and Objects layers')
 solids=bytes(bool(v) for v in layers['Collision']);spawns=[];exits=[];names=set();ids=set()
 for o in layers['Objects']:
  reject(o,{'rotation':0,'visible':True},'Object')
  if any(k in o for k in ('ellipse','polygon','polyline','text','gid','template')):raise ValueError('Unsupported object geometry')
  ident=integer(o.get('id'),1,65535,'object id')
  if ident in ids:raise ValueError('Duplicate object id')
  ids.add(ident);kind=o.get('class',o.get('type',''));name=o.get('name')
  if not isinstance(name,str) or not name or len(name)>24 or name in names:raise ValueError('Objects need unique short names')
  names.add(name)
  x=integer(o.get('x'),0,w*16-1,'object x');y=integer(o.get('y'),0,h*16-1,'object y')
  if kind=='Spawn':
   if o.get('point') is not True or o.get('width',0)!=0 or o.get('height',0)!=0 or props(o,set()):raise ValueError('Spawn needs a plain point')
   if x<5 or y<4 or x+5>w*16 or y+4>h*16:raise ValueError('Spawn foot box outside map')
   for ty in range((y-4)//16,(y+3)//16+1):
    for tx in range((x-5)//16,(x+4)//16+1):
     if solids[ty*w+tx]:raise ValueError('Spawn foot box overlaps collision')
   spawns.append((name,x,y))
  elif kind=='Exit':
   if o.get('point',False):raise ValueError('Exit needs a rectangle')
   ew=integer(o.get('width'),1,w*16,'exit width');eh=integer(o.get('height'),1,h*16,'exit height')
   if x+ew>w*16 or y+eh>h*16:raise ValueError('Exit outside map')
   for ty in range(y//16,(y+eh-1)//16+1):
    for tx in range(x//16,(x+ew-1)//16+1):
     if solids[ty*w+tx]:raise ValueError('Exit overlaps collision')
   p=props(o,{'target','spawn'})
   if not all(isinstance(p.get(k),str) and p[k] for k in ('target','spawn')):raise ValueError('Exit needs target/spawn strings')
   exits.append((name,x,y,ew,eh,p['target'],p['spawn']))
  else:raise ValueError('Unsupported object class: '+str(kind))
 if not 1<=len(spawns)<=8 or len(exits)>8:raise ValueError('Spawn/exit capacity exceeded')
 return w,h,layers,solids,spawns,exits

def compile_world(manifest):
 manifest=Path(manifest).resolve();root=manifest.parent;spec=read(manifest)
 if spec.get('version')!=1:raise ValueError('Unsupported manifest version')
 definitions=spec.get('areas',[])
 if not 1<=len(definitions)<=4:raise ValueError('Need one to four areas')
 palettes=palette(local(root,root,spec.get('palette')));all_areas=[];names={};shared=None
 for i,entry in enumerate(definitions):
  name=entry.get('id')
  if not isinstance(name,str) or not name or name in names:raise ValueError('Areas need unique IDs')
  names[name]=i;path=local(root,root,entry.get('map'));m=read(path)
  if len(m.get('tilesets',[]))!=1:raise ValueError('Profile needs one shared atlas tileset')
  art=tileset(root,path.parent,m['tilesets'][0],palettes)
  if shared is not None and art!=shared:raise ValueError('Areas must use the same prepared tileset')
  shared=art;all_areas.append(area(root,path,art[0]))
 if sum(a[0]*a[1] for a in all_areas)>MAX_CELLS:raise ValueError('Resident world cell budget exceeded')
 chunks=[]
 for w,h,layers,solids,spawns,exits in all_areas:
  chunk=bytearray(struct.pack('>4s6HI',b'LVL1',1,w,h,shared[0],len(spawns),len(exits),w*h))
  for name in ('Ground','Foreground'):chunk+=struct.pack('>'+str(w*h)+'H',*layers[name])
  chunk+=solids
  if len(chunk)&1:chunk+=b'\0'
  for i,(_,x,y) in enumerate(spawns):chunk+=struct.pack('>4H',i,x,y,0)
  for i,(_,x,y,ew,eh,target,spawn) in enumerate(exits):
   if target not in names:raise ValueError('Exit names missing target area')
   n=names[target];dest=all_areas[n][4];matches=[j for j,p in enumerate(dest) if p[0]==spawn]
   if not matches:raise ValueError('Exit names missing destination spawn')
   j=matches[0];sx,sy=dest[j][1:]
   for ex in all_areas[n][5]:
    if ex[1]<=sx<ex[1]+ex[3] and ex[2]<=sy<ex[2]+ex[4]:raise ValueError('Destination spawn is inside an exit trigger')
   chunk+=struct.pack('>8H',i,x,y,ew,eh,n,j,0)
  chunks.append(bytes(chunk))
 out=bytearray(struct.pack('>4sHHI',b'WLD1',1,len(chunks),12+8*len(chunks)+sum(map(len,chunks))))
 offset=12+8*len(chunks)
 for chunk in chunks:out+=struct.pack('>II',offset,len(chunk));offset+=len(chunk)
 out+=b''.join(chunks)
 palette_words=struct.pack('>64H',*((b<<9)|(g<<5)|(r<<1) for line in palettes for r,g,b in line))
 return {'world.bin':bytes(out),'ground.bin':shared[1],'tileattrs.bin':shared[2],'palettes.bin':palette_words}

def inputs(manifest):
 manifest=Path(manifest).resolve();root=manifest.parent;spec=read(manifest)
 paths={manifest,local(root,root,spec.get('palette'))}
 for entry in spec.get('areas',[]):
  path=local(root,root,entry.get('map'));paths.add(path)
  for ts in read(path).get('tilesets',[]):
   folder=path.parent
   if 'source' in ts:
    source=local(root,folder,ts['source']);paths.add(source);ts=read(source);folder=source.parent
   paths.add(local(root,folder,ts.get('image')))
 hashes={}
 for p in sorted(paths):
  payload=p.read_bytes()
  # Git/Tiled may use CRLF on Windows; JSON line endings do not change assets.
  if p.suffix.lower() in ('.json','.tmj','.tsj'):payload=payload.replace(b'\r\n',b'\n')
  hashes[p.relative_to(root).as_posix()]=hashlib.sha256(payload).hexdigest()
 return hashes

def verify(manifest,destination):
 destination=Path(destination);report=read(destination.with_suffix('.import.json'))
 if report.get('version')!=1 or report.get('inputs')!=inputs(manifest):raise ValueError('Tiled inputs changed; run tools/import_tiled.py manifest output with Pillow installed')
 expected=report.get('outputs',{})
 if set(expected)!=set(('world.bin','ground.bin','tileattrs.bin','palettes.bin')):raise ValueError('Incomplete import report')
 for name,digest in expected.items():
  if hashlib.sha256((destination.parent/name).read_bytes()).hexdigest()!=digest:raise ValueError('Prepared '+name+' changed; reimport original sources')
 return True

def write(manifest,destination):
 manifest=Path(manifest).resolve();before=inputs(manifest)
 files=compile_world(manifest);destination=Path(destination)
 if inputs(manifest)!=before:raise ValueError('Sources changed during import; retry from a stable snapshot')
 sources={(manifest.parent/name).resolve() for name in before}
 if any((destination.parent/name).resolve() in sources for name in [*files,'world.import.json']):raise ValueError('Output cannot replace source inputs')
 if destination.name!='world.bin':raise ValueError('Output must be named world.bin')
 destination.parent.mkdir(parents=True,exist_ok=True)
 for name,data in files.items():
  path=destination.parent/name;temp=path.with_suffix(path.suffix+'.tmp');temp.write_bytes(data);temp.replace(path)
 report={'version':1,'inputs':before,'outputs':{name:hashlib.sha256(data).hexdigest() for name,data in files.items()}}
 destination.with_suffix('.import.json').write_text(json.dumps(report,indent=2)+'\n')
 return {'areas':struct.unpack_from('>H',files['world.bin'],6)[0],'world_bytes':len(files['world.bin']),'patterns':len(files['ground.bin'])//32}
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('manifest');p.add_argument('destination');p.add_argument('--check',action='store_true');a=p.parse_args()
 try:print(json.dumps({'verified':verify(a.manifest,a.destination)} if a.check else write(a.manifest,a.destination)))
 except (ValueError,OSError,KeyError,TypeError,ImportError) as e:p.error(str(e))

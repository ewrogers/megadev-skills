"""Independent projected-hull oracle, sorting contracts and native asset packing."""
import ctypes as c,itertools,os,random,shutil,struct,subprocess,tempfile,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-isometric/assets/isometric-lab'
create=load(LAB.parent.parent/'scripts/create_project.py','iso_create').create
class Box(c.Structure):_fields_=[(n,c.c_short) for n in ['x0','y0','z0','x1','y1','z1']]
class Item(c.Structure):_fields_=[('box',Box),('id',c.c_ushort)]
class Link(c.Structure):_fields_=[('before',c.c_ushort),('after',c.c_ushort)]
def hull(box):
 x0,y0,z0,x1,y1,z1=box
 points=sorted(set((2*(x-y),x+y-2*z) for x,y,z in itertools.product((x0,x1),(y0,y1),(z0,z1))))
 def cross(a,b,d):return (b[0]-a[0])*(d[1]-a[1])-(b[1]-a[1])*(d[0]-a[0])
 lower=[];upper=[]
 for p in points:
  while len(lower)>1 and cross(lower[-2],lower[-1],p)<=0:lower.pop()
  lower.append(p)
 for p in reversed(points):
  while len(upper)>1 and cross(upper[-2],upper[-1],p)<=0:upper.pop()
  upper.append(p)
 return lower[:-1]+upper[:-1]
def overlap(a,b):
 for polygon in (a,b):
  for p,q in zip(polygon,polygon[1:]+polygon[:1]):
   nx,ny=q[1]-p[1],p[0]-q[0];va=[x*nx+y*ny for x,y in a];vb=[x*nx+y*ny for x,y in b]
   if max(va)<=min(vb) or max(vb)<=min(va):return False
 return True
class IsometricTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cc:raise unittest.SkipTest('Host C17 compiler required')
  path=Path(cls.tmp.name)/('iso.dll' if os.name=='nt' else 'iso.so')
  subprocess.run([cc,'-std=c17','-O3','-shared',*(['-fPIC'] if os.name!='nt' else []),*(str(LAB/'src'/n) for n in ['iso.c','world.c','controller.c','packed.c']),'-o',str(path)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(path))
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(lambda:_ctypes.FreeLibrary(cls.lib._handle))
  cls.lib.iso_relation.argtypes=[Box,Box]
  cls.lib.iso_sort.argtypes=[c.POINTER(Item),c.c_ushort,c.POINTER(Link),c.c_ushort,c.POINTER(c.c_ushort)]
  cls.lib.iso_unpack.argtypes=[c.c_void_p,c.c_ushort,c.c_void_p,c.c_ushort]
 def test_6000_relations_against_independent_convex_hulls(self):
  rng=random.Random(20261019)
  for _ in range(6000):
   boxes=[]
   for n in range(2):
    x,y,z=rng.randint(-2000,2000),rng.randint(-2000,2000),rng.randint(0,1200)
    boxes.append((x,y,z,x+rng.randint(16,1000),y+rng.randint(16,1000),z+rng.randint(16,800)))
   a,b=boxes;before=a[3]<=b[0] or a[4]<=b[1] or a[5]<=b[2];after=b[3]<=a[0] or b[4]<=a[1] or b[5]<=a[2]
   expected=0 if not overlap(hull(a),hull(b)) else 1 if before and not after else 2 if after and not before else 3
   self.assertEqual(self.lib.iso_relation(Box(*a),Box(*b)),expected)
 def test_explicit_dags_respect_edges_and_stable_id_ties(self):
  rng=random.Random(0x534344)
  for _ in range(100):
   ids=list(range(12));rng.shuffle(ids);items=(Item*12)(*(Item(Box(i*256,0,0,i*256+16,16,16),ids[i]) for i in range(12)))
   rank=ids.copy();rng.shuffle(rank);edges=[(rank[i],rank[j]) for i in range(12) for j in range(i+1,12) if rng.random()<0.2][:24]
   links=(Link*len(edges))(*(Link(a,b) for a,b in edges));out=(c.c_ushort*24)()
   self.assertEqual(self.lib.iso_sort(items,12,links,len(edges),out),0)
   remaining=set(ids);expected=[]
   while remaining:
    ready=[n for n in remaining if not any(b==n and a in remaining for a,b in edges)];pick=min(ready);expected.append(pick);remaining.remove(pick)
   self.assertEqual([items[i].id for i in out[:12]],expected)
 def test_standalone_contract_and_native_assets(self):
  with tempfile.TemporaryDirectory() as folder:
   root=create(Path(folder)/'isometric game')
   with self.assertRaises(ValueError):create(root)
   self.assertTrue(load(root/'tools/check_isometric.py','iso_check').check()['passed'])
   art=load(root/'tools/make_art.py','iso_art_pack');files,header=art.build()
   for name,data in files.items():self.assertEqual(data,(root/'res'/name).read_bytes())
   self.assertEqual(header,(root/'src/art_meta.h').read_text())
   self.assertTrue(all(v&~0xeee==0 for v in struct.unpack('>64H',files['palettes.bin'])))
   for name in ['actors','ground','map']:
    raw=files[name+'.bin'];packed=(root/'res'/(name+'.rle')).read_bytes();out=c.create_string_buffer(len(raw))
    self.assertEqual(self.lib.iso_unpack(packed,len(packed),out,len(raw)),1);self.assertEqual(out.raw,raw)
    self.assertFalse(self.lib.iso_unpack(packed,len(packed)-1,out,len(raw)))
   self.assertLess(640+len(files['ground.bin'])//32,1280)
   # Reconstruct all original ground pixels through independent map/tile indexing.
   ground=art.art.ground();indices=struct.unpack('>2048H',files['map.bin']);tiles=files['ground.bin']
   for y in range(256):
    for x in range(512):
     index=indices[(y//8)*64+x//8];v=tiles[index*32+(y%8)*4+(x%8)//2];pixel=(v>>4) if x%2==0 else (v&15)
     self.assertEqual(pixel,ground.p[y*512+x])
 def test_packed_decode_rejects_truncated_and_overlong_packets(self):
  out=c.create_string_buffer(b'ABCD',5)
  for data,capacity in [(b'\x80',1),(b'\x03ab',4),(b'\x84x',4),(b'\x80x\x80y',1)]:self.assertFalse(self.lib.iso_unpack(data,len(data),out,capacity))

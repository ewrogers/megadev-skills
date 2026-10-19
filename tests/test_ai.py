"""Independent shortest-path oracle, malformed inputs and standalone skill use."""
import collections,ctypes as c,os,random,shutil,struct,subprocess,sys,tempfile,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-ai/assets/enemy-lab'
create=load(LAB.parent.parent/'scripts/create_project.py','ai_create').create
class Map(c.Structure):_fields_=[('solid',c.c_ubyte*240),('revision',c.c_ushort)]
class Search(c.Structure):_fields_=[('parent',c.c_ubyte*240),('queue',c.c_ubyte*240)]+[(n,c.c_ushort) for n in ['head','tail','status','start','goal','owner','generation','revision','expanded','last_work']]
class AiTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cc:raise unittest.SkipTest('Host C17 compiler required')
  path=Path(cls.tmp.name)/('ai.dll' if os.name=='nt' else 'ai.so')
  subprocess.run([cc,'-std=c17','-O3','-shared',*(['-fPIC'] if os.name!='nt' else []),str(LAB/'src/enemy.c'),'-o',str(path)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(path))
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(lambda:_ctypes.FreeLibrary(cls.lib._handle))
  cls.lib.ai_begin.argtypes=[c.POINTER(Search),c.POINTER(Map)]+[c.c_ushort]*4
  cls.lib.ai_work.argtypes=[c.POINTER(Search),c.POINTER(Map),c.c_ushort]
  cls.lib.ai_path.argtypes=[c.POINTER(Search),c.POINTER(c.c_ubyte),c.c_ushort]
  cls.lib.ai_result_valid.argtypes=[c.POINTER(Search),c.POINTER(Map),c.c_ushort,c.c_ushort]
  cls.lib.ai_ray.argtypes=[c.POINTER(Map)]+[c.c_short]*4
 def test_paths_match_independent_oracle_and_respect_budget(self):
  rng=random.Random(0x534344)
  for case in range(80):
   m=Map();m.revision=case+1
   for n in range(240):m.solid[n]=rng.random()<0.28
   start,goal=rng.randrange(240),rng.randrange(240);m.solid[start]=m.solid[goal]=0
   distance={start:0};todo=collections.deque([start])
   while todo:
    n=todo.popleft();x,y=n%20,n//20
    for xx,yy in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
     if 0<=xx<20 and 0<=yy<12:
      k=yy*20+xx
      if not m.solid[k] and k not in distance:distance[k]=distance[n]+1;todo.append(k)
   q=Search();self.assertTrue(self.lib.ai_begin(c.byref(q),c.byref(m),start,goal,2,7))
   for _ in range(241):
    if q.status!=1:break
    self.assertLessEqual(self.lib.ai_work(c.byref(q),c.byref(m),65535),12)
   self.assertLessEqual(q.tail,240);self.assertLessEqual(q.expanded,240)
   if goal not in distance:self.assertEqual(q.status,3);continue
   self.assertEqual(q.status,2);out=(c.c_ubyte*240)();length=self.lib.ai_path(c.byref(q),out,240)
   self.assertEqual(length,distance[goal]);n=start
   for k in list(out)[:length]:
    self.assertFalse(m.solid[k]);self.assertEqual(abs(k%20-n%20)+abs(k//20-n//20),1);n=k
   self.assertEqual(n,goal)
 def test_busy_workspace_cancellation_and_stale_results(self):
  m=Map();m.revision=4;q=Search()
  self.assertTrue(self.lib.ai_begin(c.byref(q),c.byref(m),0,239,1,9));before=bytes(q)
  self.assertFalse(self.lib.ai_begin(c.byref(q),c.byref(m),2,3,2,9));self.assertEqual(bytes(q),before)
  m.revision+=1;self.assertEqual(self.lib.ai_work(c.byref(q),c.byref(m),12),0);self.assertEqual(q.status,5)
  self.assertTrue(self.lib.ai_begin(c.byref(q),c.byref(m),5,5,1,9))
  self.assertTrue(self.lib.ai_result_valid(c.byref(q),c.byref(m),1,9))
  self.assertFalse(self.lib.ai_result_valid(c.byref(q),c.byref(m),1,10))
  for invalid in [240,255,65535]:self.assertFalse(self.lib.ai_begin(c.byref(q),c.byref(m),invalid,5,1,9))
 def test_occlusion_and_corner_touching_are_symmetric(self):
  m=Map();m.solid[1]=m.solid[20]=1
  self.assertFalse(self.lib.ai_ray(c.byref(m),8,8,24,24));self.assertFalse(self.lib.ai_ray(c.byref(m),24,24,8,8))
  for p in [(-1,8,24,24),(8,8,320,24),(8,8,24,192)]:self.assertFalse(self.lib.ai_ray(c.byref(m),*p))
 def test_standalone_contract_and_original_assets(self):
  with tempfile.TemporaryDirectory() as d:
   root=create(Path(d)/'enemy game')
   with self.assertRaises(ValueError):create(root)
   with self.assertRaises(ValueError):create(Path(d)/'bad',project_id='../bad')
   check=load(root/'tools/check_enemy.py','ai_portable');self.assertTrue(check.check()['passed'])
   art=load(root/'tools/make_art.py','ai_art');files,header=art.build()
   for name,data in files.items():self.assertEqual(data,(root/'res'/name).read_bytes())
   self.assertEqual(header,(root/'src/art_meta.h').read_text())
   self.assertEqual(len(files['actors.bin']),188*32);self.assertEqual(len(files['ground.bin']),24*32)
   self.assertTrue(all(v&~0xeee==0 for v in struct.unpack('>64H',files['palettes.bin'])))

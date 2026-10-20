"""Portable roster behavior, generated assets and deterministic randomized play."""
import ctypes as c,os,random,shutil,struct,subprocess,tempfile,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-multiplayer/assets/co-op-lab'
create=load(LAB.parent.parent/'scripts/create_project.py','mp_create').create
class Player(c.Structure):_fields_=[('x',c.c_short),('y',c.c_short)]+[(n,c.c_ushort) for n in 'socket kind active connected hp hurt respawn cooldown pulse face walk moving grace leave'.split()]
class Enemy(c.Structure):_fields_=[('x',c.c_short),('y',c.c_short),('stun',c.c_ushort),('direction',c.c_ushort)]
class Game(c.Structure):_fields_=[('players',Player*4),('enemies',Enemy*3)]+[(n,c.c_ushort) for n in 'ticks remainder paused started demo events coins complete clamped'.split()]+[('camera_x',c.c_short),('camera_y',c.c_short)]
class Sample(c.Structure):_fields_=[(n,c.c_ushort) for n in ['kind','status','held']]+[('dx',c.c_short),('dy',c.c_short)]
class MultiplayerTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cc:raise unittest.SkipTest('C17 compiler required')
  out=Path(cls.tmp.name)/('mp.dll' if os.name=='nt' else 'mp.so')
  subprocess.run([cc,'-std=c17','-O3','-shared',*(['-fPIC'] if os.name!='nt' else []),str(LAB/'src/coop.c'),str(LAB/'src/input.c'),'-o',str(out)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(out))
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(lambda:_ctypes.FreeLibrary(cls.lib._handle))
  cls.lib.mp_advance.argtypes=[c.POINTER(Game),c.c_ushort,c.POINTER(Sample),c.POINTER(c.c_ushort)]
 def test_standalone_contract_and_assets(self):
  with tempfile.TemporaryDirectory() as d:
   root=create(Path(d)/'co op')
   with self.assertRaises(ValueError):create(root)
   self.assertTrue(load(root/'tools/check_coop.py','mp_check').check()['passed'])
   art=load(root/'tools/make_art.py','mp_art').build()
   for n,b in art.items():self.assertEqual(b,(root/'res'/n).read_bytes())
   self.assertEqual(len(art['actors.bin']),92*32)
   self.assertTrue(all(v&~0xeee==0 for v in struct.unpack('>64H',art['palettes.bin'])))
 def test_random_rosters_remain_unique_bounded_and_visible(self):
  rng=random.Random(0x434f4f50);g=Game();self.lib.mp_init(c.byref(g));s=(Sample*8)();hits=(c.c_ushort*8)()
  for frame in range(6000):
   for i in range(8):
    connected=rng.random()>.02;s[i].kind=1 if connected else 0;s[i].status=0 if connected else 1;s[i].held=rng.choice([0,1,2,4,8,5,10]);hits[i]=128 if rng.random()<.002 else 0
   self.lib.mp_advance(c.byref(g),6 if frame&1 else 5,s,hits)
   claims=[p.socket for p in g.players if p.active];self.assertEqual(len(claims),len(set(claims)))
   self.assertLessEqual(g.camera_x,192);self.assertLessEqual(g.camera_y,80)
   for p in g.players:
    if p.active and p.connected and p.hp:
     self.assertTrue(self.lib.mp_valid(p.x,p.y));self.assertTrue(12<=p.x//4-g.camera_x<308);self.assertTrue(20<=p.y//4-g.camera_y<164,(frame,p.y,g.camera_y,g.paused,g.started,[(v.x,v.y,v.connected,v.hp) for v in g.players if v.active]))
 def test_disconnected_pads_cannot_move_or_false_join_on_first_sample(self):
  # in_update layout and edge semantics come from the same input owner used by the Lab.
  slot=(c.c_ushort*13)();s=Sample(1,0,128,0,0)
  self.lib.in_reset(slot);self.lib.in_update(slot,c.byref(s),60);self.assertEqual(slot[1],0)
  s.held=0;self.lib.in_update(slot,c.byref(s),60);s.held=128;self.lib.in_update(slot,c.byref(s),60);self.assertEqual(slot[1],128)
  s.status=2;self.lib.in_update(slot,c.byref(s),60);self.assertEqual(slot[0],0)

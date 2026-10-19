"""Independent contact/guard oracles and self-contained skill installation."""
import ctypes as c,os,random,shutil,struct,subprocess,tempfile,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-combat/assets/combat-lab'
create=load(LAB.parent.parent/'scripts/create_project.py','combat_create').create
class Box(c.Structure):_fields_=[(n,c.c_short) for n in ['x','y','w','h']]
class Actor(c.Structure):_fields_=[(n,c.c_short) for n in ['x','y','kx','ky']]+[(n,c.c_ushort) for n in ['face','state','age','hp','iframes','guard','flash','cooldown','serial','generation','joined','moving','victims']]
class CombatTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup)
  cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cc:raise unittest.SkipTest('Host C17 compiler required')
  path=Path(cls.tmp.name)/('combat.dll' if os.name=='nt' else 'combat.so')
  subprocess.run([cc,'-std=c17','-O3','-shared',*(['-fPIC'] if os.name!='nt' else []),str(LAB/'src/combat.c'),'-o',str(path)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(path))
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(lambda:_ctypes.FreeLibrary(cls.lib._handle))
  cls.lib.co_overlap.argtypes=[c.POINTER(Box)]*2
  cls.lib.co_front.argtypes=[c.POINTER(Actor),c.c_short,c.c_short]
  cls.lib.co_attack_box.argtypes=[c.POINTER(Actor),c.POINTER(Box)]
 def test_overlap_independent_interval_oracle(self):
  rng=random.Random(0x534344)
  for _ in range(1000):
   a,b=[Box(*(rng.randrange(-320,321) for _ in range(2)),*(rng.randrange(-2,41) for _ in range(2))) for n in range(2)]
   expected=a.w>0 and a.h>0 and b.w>0 and b.h>0 and max(a.x,b.x)<min(a.x+a.w,b.x+b.w) and max(a.y,b.y)<min(a.y+a.h,b.y+b.h)
   self.assertEqual(bool(self.lib.co_overlap(c.byref(a),c.byref(b))),expected)
 def test_directional_guards_against_dot_product_oracle(self):
  for face,(fx,fy) in enumerate([(0,1),(0,-1),(-1,0),(1,0)]):
   a=Actor();a.x=100;a.y=80;a.face=face
   for dx in range(-9,10):
    for dy in range(-9,10):
     forward=dx*fx+dy*fy;side=dx*(-fy)+dy*fx
     self.assertEqual(bool(self.lib.co_front(c.byref(a),100+dx,80+dy)),forward>0 and abs(side)<=forward)
 def test_active_windows_and_all_facing_boxes(self):
  for face in range(4):
   a=Actor();a.x=100;a.y=80;a.state=1;a.face=face
   for age in range(25):
    a.age=age;b=Box();self.assertEqual(bool(self.lib.co_attack_box(c.byref(a),c.byref(b))),6<=age<13)
    if 6<=age<13:
     self.assertTrue(b.x<=100<=b.x+b.w or face in (2,3))
     self.assertEqual((b.w,b.h),(32,34) if face>=2 else (34,32))
 def test_standalone_contract_and_art(self):
  with tempfile.TemporaryDirectory() as folder:
   root=create(Path(folder)/'combat game')
   with self.assertRaises(ValueError):create(root)
   with self.assertRaises(ValueError):create(Path(folder)/'bad',project_id='../bad')
   self.assertTrue(load(root/'tools/check_combat.py','combat_check').check()['passed'])
   art=load(root/'tools/make_art.py','combat_art');files,header=art.build()
   for face in range(4):
    shield=art.shield(face);self.assertEqual(len(shield),16);self.assertTrue(all(len(row)==16 for row in shield))
   for name,data in files.items():self.assertEqual(data,(root/'res'/name).read_bytes())
   self.assertEqual(header,(root/'src/art_meta.h').read_text())
   self.assertLess(128+len(files['actors.bin'])//32,640)
   self.assertTrue(all(v&~0xeee==0 for v in struct.unpack('>64H',files['palettes.bin'])))

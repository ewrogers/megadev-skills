"""Independent arithmetic oracle, source generation and installed host checks."""
import ctypes as c
import math
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-math/assets/math-lab'
class Vec(c.Structure):_fields_=[('x',c.c_short),('y',c.c_short)]
def trunc(n,d):return (-1 if n<0 else 1)*(abs(n)//d)
class MathTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory();cls.root=Path(cls.temp.name)
  cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
  if not cc:raise unittest.SkipTest('Host C17 compiler needed')
  path=cls.root/('math.dll' if sys.platform=='win32' else 'math.so')
  r=subprocess.run([cc,'-shared','-DMATH_HOST_DLL','-O3',*(['-fPIC'] if sys.platform!='win32' else []),str(LAB/'src/math_core.c'),'-o',str(path)],capture_output=True,text=True)
  if r.returncode:raise RuntimeError(r.stderr)
  cls.lib=c.CDLL(str(path))
  for n in ['add','sub','mul','div']:
   f=getattr(cls.lib,'mm_'+n);f.argtypes=[c.c_short,c.c_short,c.POINTER(c.c_short)];f.restype=c.c_ushort
  cls.lib.mm_isqrt.argtypes=[c.c_uint];cls.lib.mm_isqrt.restype=c.c_ushort
  cls.lib.mm_sin.argtypes=cls.lib.mm_cos.argtypes=[c.c_ushort];cls.lib.mm_sin.restype=cls.lib.mm_cos.restype=c.c_short
  cls.lib.mm_normalize.argtypes=[c.POINTER(Vec),c.POINTER(Vec)];cls.lib.mm_normalize.restype=c.c_ushort
  cls.lib.mm_lerp.argtypes=[c.c_short,c.c_short,c.c_ushort,c.POINTER(c.c_short)]
  cls.lib.mm_smoothstep.argtypes=[c.c_ushort,c.POINTER(c.c_ushort)]
 @classmethod
 def tearDownClass(cls):
  if sys.platform=='win32':
   import _ctypes
   _ctypes.FreeLibrary(cls.lib._handle)
  cls.temp.cleanup()
 def test_checked_arithmetic_against_integer_oracle(self):
  rng=random.Random(704);edge=[-32768,-32767,-257,-256,-1,0,1,255,256,32767]
  for a,b in [(a,b) for a in edge for b in edge]+[(rng.randint(-32768,32767),rng.randint(-32768,32767)) for _ in range(3000)]:
   for name,expected in [('add',a+b),('sub',a-b),('mul',trunc(a*b,256)),('div',trunc(a*256,abs(b))*(-1 if b<0 else 1) if b else None)]:
    out=c.c_short(12345);ok=getattr(self.lib,'mm_'+name)(a,b,c.byref(out));valid=expected is not None and -32768<=expected<=32767
    self.assertEqual(bool(ok),valid,(name,a,b));self.assertEqual(out.value,expected if valid else 12345,(name,a,b))
 def test_trig_all_angles_and_sqrt_extremes(self):
  rng=random.Random(33)
  for angle in range(1024):
   self.assertLessEqual(abs(self.lib.mm_sin(angle)-math.sin(angle*math.tau/1024)*256),0.501)
   self.assertEqual(self.lib.mm_sin(angle),self.lib.mm_sin(angle+1024))
   self.assertEqual(self.lib.mm_cos(angle),self.lib.mm_sin((angle+256)%1024))
  for n in [0,1,2,3,4,65535,65536,2147483648,4294967295]+[rng.randrange(1<<32) for _ in range(1000)]:self.assertEqual(self.lib.mm_isqrt(n),math.isqrt(n))
 def test_normalization_tiny_vectors_and_aliasing(self):
  rng=random.Random(9)
  for x,y in [(0,0),(1,1),(-1,1),(1,0),(-32768,-32768),(32767,1)]+[(rng.randint(-32768,32767),rng.randint(-32768,32767)) for _ in range(1000)]:
   v=Vec(x,y);out=Vec(123,45);ok=self.lib.mm_normalize(c.byref(v),c.byref(out))
   self.assertEqual(bool(ok),bool(x or y))
   if not ok:self.assertEqual((out.x,out.y),(123,45));continue
   length=math.hypot(x,y);self.assertLessEqual(abs(out.x-x*256/length),1.02);self.assertLessEqual(abs(out.y-y*256/length),1.02)
   self.assertTrue(self.lib.mm_normalize(c.byref(v),c.byref(v)));self.assertEqual((v.x,v.y),(out.x,out.y))
 def test_interpolation_endpoints_monotonicity_full_range(self):
  for a,b in [(-32768,32767),(32767,-32768),(-123,456),(6,6)]:
   previous=a
   for t in range(257):
    out=c.c_short();self.assertTrue(self.lib.mm_lerp(a,b,t,c.byref(out)));self.assertEqual(out.value,a+trunc((b-a)*t,256))
    self.assertTrue(previous<=out.value if b>=a else previous>=out.value);previous=out.value
   self.assertEqual(previous,b)
  for t in range(258):
   out=c.c_ushort(999);ok=self.lib.mm_smoothstep(t,c.byref(out));self.assertEqual(bool(ok),t<=256)
   self.assertEqual(out.value,(t*t*(768-2*t))//65536 if t<=256 else 999)
 def test_generator_installed_checks_and_art_reproduction(self):
  skill=LAB.parents[1];create=load(skill/'scripts/create_project.py','math_creator').create
  with tempfile.TemporaryDirectory() as d:
   game=create(Path(d)/'math game')
   r=subprocess.run([sys.executable,str(game/'tools/check_math.py')],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
   with self.assertRaises(ValueError):create(game)
  art=load(LAB/'tools/make_art.py','math_art')
  for name,data in art.build().items():self.assertEqual(data,(LAB/'res'/name).read_bytes())
  palettes=struct.unpack('>64H',(LAB/'res/palettes.bin').read_bytes());self.assertTrue(all(not w&~0xEEE for w in palettes))

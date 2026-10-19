"""Effect behavior, hardware admission, native art and sanitizer checks."""
import ctypes as c
from pathlib import Path
import os,shutil,struct,subprocess,sys,tempfile,unittest
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-effects/assets/effects-lab'
class Particle(c.Structure):_fields_=[(n,c.c_short) for n in ['x','y','vx','vy']]+[(n,c.c_ushort) for n in ['life','kind']]
class World(c.Structure):_fields_=[('p',Particle*64)]+[(n,c.c_ushort) for n in ['x','y','tick','remainder','seed','weather','density','paused','shake','left','denied','clamped']]
class View(c.Structure):_fields_=[('sat',(c.c_ushort*4)*56)]+[(n,c.c_ushort) for n in ['count','omitted','culled']]+[(n,c.c_short) for n in ['shake_x','shake_y']]+[('bands',c.c_ubyte*28)]
class EffectsTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.path=Path(cls.tmp.name);cls.cc=shutil.which('clang') or shutil.which('cc')
  if not cls.cc:
   if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
   raise unittest.SkipTest('C17 compiler required')
  p=cls.path/('effects.dll' if sys.platform=='win32' else 'effects.so')
  subprocess.run([cls.cc,'-shared','-O3',*(['-fPIC'] if sys.platform!='win32' else []),str(LAB/'src/fx.c'),'-o',str(p)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(p));cls.lib.fx_init.argtypes=[c.POINTER(World)];cls.lib.fx_burst.argtypes=[c.POINTER(World)]
  cls.lib.fx_advance.argtypes=[c.POINTER(World),c.c_ushort,c.c_ushort,c.c_ushort];cls.lib.fx_build.argtypes=[c.POINTER(World),c.POINTER(View)]
 @classmethod
 def tearDownClass(cls):
  if sys.platform=='win32':
   import _ctypes
   _ctypes.FreeLibrary(cls.lib._handle)
  cls.tmp.cleanup()
 def world(self):
  w=World();self.lib.fx_init(c.byref(w));return w
 def step(self,w,units=5,held=0,hit=0):return self.lib.fx_advance(c.byref(w),units,held,hit)
 def view(self,w):
  v=View();self.lib.fx_build(c.byref(w),c.byref(v));return v
 def test_real_second_matches_pal_ntsc_and_pause(self):
  a=self.world();b=self.world()
  for _ in range(60):self.step(a,5,8)
  for _ in range(50):self.step(b,6,8)
  self.assertEqual(bytes(a),bytes(b));self.assertEqual(a.tick,60)
  self.step(a,5,0,128);before=bytes(a)
  for _ in range(60):self.step(a,6,8,64)
  self.assertEqual(bytes(a),before);self.step(a,5,0,128);self.assertEqual(a.tick,61)
 def test_partitions_lifetimes_admission_and_edge_actions(self):
  w=self.world()
  for _ in range(60):self.step(w)
  weather=bytes(w.p)[32*12:]
  for _ in range(8):self.lib.fx_burst(c.byref(w))
  self.assertEqual(bytes(w.p)[32*12:],weather);self.assertEqual(sum(p.life>0 for p in w.p[:24]),24);self.assertGreater(w.denied,0)
  w.weather=0
  for _ in range(60):self.step(w)
  self.assertFalse(any(p.life for p in w.p[:24]));self.assertFalse(any(p.life for p in w.p[32:]))
  self.step(w,65535);self.assertEqual(w.clamped,1);self.assertEqual(w.remainder,0)
  self.step(w,0,1,128);self.assertEqual(w.shake,0);self.assertEqual(w.paused,0)
 def test_sat_limits_order_links_culling_and_no_mutation(self):
  w=self.world()
  for i,p in enumerate(w.p):p.x=(12+(i%12)*25)*16;p.y=(50+(i//12)*26)*16;p.life=20;p.kind=1 if i<24 else 3 if i<32 else 5
  before=bytes(w);v=self.view(w);self.assertEqual(bytes(w),before);self.assertEqual(v.count,56);self.assertGreater(v.omitted,0)
  self.assertEqual(v.sat[0][2]&2047,128)
  for i in range(v.count):self.assertEqual(v.sat[i][1]&127,i+1 if i+1<v.count else 0)
  self.assertTrue(all(n<=16 for n in v.bands))
  # Force every effect onto one band. Hero remains, weather loses admission.
  for p in w.p:p.x=50*16;p.y=96*16
  v=self.view(w);self.assertLessEqual(v.count,17);self.assertGreater(v.omitted,30)
  for line in range(224):
   entries=pixels=0
   for row in v.sat[:v.count]:
    y=row[0]-128;h=(((row[1]>>8)&3)+1)*8;width=(((row[1]>>10)&3)+1)*8
    if y<=line<y+h:entries+=1;pixels+=width
   self.assertLessEqual(entries,20);self.assertLessEqual(pixels,320)
  for p in w.p:p.x=-128
  v=self.view(w);self.assertEqual(v.count,1);self.assertEqual(v.culled,64)
 def test_shake_is_presentation_and_restores(self):
  w=self.world();self.lib.fx_burst(c.byref(w));before=bytes(w);v=self.view(w)
  self.assertEqual(bytes(w),before);self.assertLessEqual(abs(v.shake_x),2)
  for _ in range(18):self.step(w)
  v=self.view(w);self.assertEqual((v.shake_x,v.shake_y),(0,0))
 def test_art_and_standalone_generator(self):
  art=load(LAB/'tools/make_art.py','effects_art')
  for n,data in art.build().items():self.assertEqual(data,(LAB/'res'/n).read_bytes())
  words=struct.unpack('>64H',(LAB/'res/palettes.bin').read_bytes());self.assertTrue(all(not v&~0xEEE for v in words))
  tiles=len((LAB/'res/ground.bin').read_bytes())//32;mapping=struct.unpack('>1120H',(LAB/'res/map.bin').read_bytes());self.assertTrue(all(256<=v&2047<256+tiles for v in mapping));self.assertLess(256+tiles,0xB800//32)
  create=load(LAB.parents[1]/'scripts/create_project.py','effects_creator').create
  with tempfile.TemporaryDirectory() as d:
   game=create(Path(d)/'effect game');self.assertTrue((game/'src/fx.c').is_file())
   with self.assertRaises(ValueError):create(game)
 def test_sanitized_combined_stress_o0_o3(self):
  harness=self.path/'stress.c';harness.write_text('''#include "fx.h"
#include <assert.h>
int main(void){FxWorld w;FxView v;fx_init(&w);for(unsigned i=0;i<10000;i++){fx_advance(&w,i&1?5:6,i&15,i%8==0?64:i%111==0?16:i%133==0?32:0);fx_build(&w,&v);assert(v.count<=56);for(unsigned b=0;b<28;b++)assert(v.bands[b]<=16);}return 0;}''')
  for opt in ['-O0','-O3']:
   p=self.path/('stress'+opt[-1]+('.exe' if sys.platform=='win32' else ''))
   subprocess.run([self.cc,opt,*(['-fsanitize=undefined','-fno-sanitize-recover=all'] if os.name!='nt' else []),'-I'+str(LAB/'src'),str(harness),str(LAB/'src/fx.c'),'-o',str(p)],check=True,capture_output=True)
   subprocess.run([str(p)],check=True,capture_output=True)

"""Portable raster plans, native assets and region-independent simulation."""
import ctypes as c
import os,shutil,struct,subprocess,sys,tempfile,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-raster/assets/raster-lab'
class Plan(c.Structure):_fields_=[(n,c.c_ushort) for n in ['line','flags','scroll_a','scroll_b','table','wet0','wet1','enabled','top_a','top_b']]
class World(c.Structure):_fields_=[('x',c.c_ushort*2),('y',c.c_ushort*2)]+[(n,c.c_ushort) for n in ['line','tick','remainder','selected','enabled','waves','automatic','paused','demo','direction','clamped']]
class View(c.Structure):_fields_=[('plan',Plan),('scroll',(c.c_ushort*2)*224),('sat',(c.c_ushort*4)*18),('count',c.c_ushort)]
class RasterTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory();cls.root=Path(cls.temp.name);cls.addClassCleanup(cls.temp.cleanup)
  cls.cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cls.cc:
   if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
   raise unittest.SkipTest('C17 compiler required')
  lib=cls.root/('raster.dll' if os.name=='nt' else 'raster.so')
  subprocess.run([cls.cc,'-shared','-DRASTER_HOST_DLL','-O3',*(['-fPIC'] if os.name!='nt' else []),str(LAB/'src/raster.c'),'-o',str(lib)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(lib))
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(_ctypes.FreeLibrary,cls.lib._handle)
  cls.lib.rt_init.argtypes=[c.POINTER(World),c.c_ushort];cls.lib.rt_advance.argtypes=[c.POINTER(World),c.c_ushort,c.c_ushort,c.c_ushort]
  cls.lib.rt_build.argtypes=[c.POINTER(World),c.POINTER(View)];cls.lib.rt_plan_valid.argtypes=[c.POINTER(Plan)];cls.lib.rt_plan_valid.restype=c.c_ushort
 def world(self,demo=0):
  w=World();self.lib.rt_init(c.byref(w),demo);return w
 def advance(self,w,units=5,held=0,hit=0):return self.lib.rt_advance(c.byref(w),units,held,hit)
 def view(self,w):
  v=View();before=bytes(w);self.lib.rt_build(c.byref(w),c.byref(v));self.assertEqual(before,bytes(w));self.assertTrue(self.lib.rt_plan_valid(c.byref(v.plan)));return v
 def test_pal_ntsc_pause_and_input_edges(self):
  for demo in [0,1]:
   a=self.world(demo);b=self.world(demo)
   for i in range(60):self.advance(a,5,8,32 if i==0 else 0)
   for i in range(50):self.advance(b,6,8,32 if i==0 else 0)
   self.assertEqual(bytes(a),bytes(b));self.assertEqual(a.tick,60)
   self.advance(a,5,0,128);frozen=bytes(a)
   for _ in range(50):self.advance(a,6,15,112)
   self.assertEqual(bytes(a),frozen)
   self.advance(a,5,0,128);self.assertEqual(a.tick,61)
   self.assertEqual(self.advance(a,65535),4);self.assertEqual(a.clamped,1)
 def test_water_sweep_bounds_and_reflection_coordinates(self):
  w=self.world();self.advance(w,0,0,32)
  for _ in range(800):
   self.advance(w);v=self.view(w);self.assertTrue(112<=w.line<=176)
   for y in range(w.line,224):self.assertEqual(255-((y+v.plan.scroll_a)&255),2*w.line-y)
   self.assertEqual(v.plan.scroll_b,0)
  self.advance(w,0,0,16);v=self.view(w)
  self.assertTrue(all(r[0]==r[1] for r in v.scroll))
  self.advance(w,0,0,64);self.assertEqual(self.view(w).plan.flags,1)
 def test_ball_reflection_flips_cells_and_tracks_surface(self):
  w=self.world();w.waves=0
  for line in [144,160,176]:
   w.line=line;v=self.view(w)
   source={r[2]&2047:r for r in v.sat[10:v.count] if not r[2]&0x1000}
   reflected=[r for r in v.sat[10:v.count] if r[2]&0x1000]
   self.assertEqual(len(reflected),2 if line==176 else 4)
   for r in reflected:
    original=source[r[2]&2047]
    self.assertEqual(r[3],original[3]);self.assertEqual(r[0]-128,2*line-(original[0]-128)-8)
    self.assertEqual((r[2]>>13)&3,3)
  w.line=144;w.waves=1
  for tick in range(64):
   w.tick=tick;v=self.view(w)
   original={r[2]&2047:r for r in v.sat[10:v.count] if not r[2]&0x1000}
   offsets={r[3]-original[r[2]&2047][3] for r in v.sat[10:v.count] if r[2]&0x1000}
   self.assertEqual(len(offsets),1,'Reflected cells must move as one ball')
  w.line=112;v=self.view(w)
  self.assertFalse(any(r[2]&0x1000 for r in v.sat[10:v.count]))
 def test_split_cameras_shared_map_and_disabled_comparison(self):
  w=self.world(1);v=self.view(w);first=bytes(v.scroll)
  self.advance(w,5,8);v=self.view(w)
  self.assertNotEqual(bytes(v.scroll)[:512],first[:512]);self.assertEqual(bytes(v.scroll)[512:],first[512:])
  self.advance(w,0,0,64);self.assertEqual(w.selected,1)
  before=w.y[0]
  for _ in range(30):self.advance(w,5,1)
  self.assertEqual(w.y[0],before);v=self.view(w)
  for i,screen in [(0,32),(1,136)]:
   offset=v.plan.top_b if i==0 else v.plan.scroll_b
   self.assertEqual((screen+offset)&255,w.y[i]-48)
  self.advance(w,0,0,16);self.assertEqual(self.view(w).plan.enabled,0)
 def test_hardware_sprite_bounds_and_links(self):
  for demo in [0,1]:
   w=self.world(demo)
   for line in range(112,177):
    w.line=line;v=self.view(w);self.assertLessEqual(v.count,18)
    for i,r in enumerate(v.sat[:v.count]):self.assertEqual(r[1]&127,i+1 if i+1<v.count else 0)
    for y in range(224):
     entries=pixels=0
     for r in v.sat[:v.count]:
      yy=r[0]-128;h=(((r[1]>>8)&3)+1)*8
      if yy<=y<yy+h:entries+=1;pixels+=(((r[1]>>10)&3)+1)*8
     self.assertLessEqual(entries,20);self.assertLessEqual(pixels,320)
 def test_invalid_plan_rejected(self):
  p=self.view(self.world()).plan
  for field,value in [('line',111),('line',177),('flags',4),('table',0x8230),('scroll_a',512),('enabled',2),('wet0',1),('top_b',512)]:
   q=Plan.from_buffer_copy(p);setattr(q,field,value);self.assertFalse(self.lib.rt_plan_valid(c.byref(q)),(field,value))
  self.assertFalse(self.lib.rt_plan_valid(None))
 def test_art_reconstruction_reflection_and_safe_generation(self):
  art=load(LAB/'tools/make_art.py','raster_art')
  for n,data in art.build().items():self.assertEqual(data,(LAB/'res'/n).read_bytes())
  dry=struct.unpack('>2048H',(LAB/'res/water_map.bin').read_bytes());wet=struct.unpack('>2048H',(LAB/'res/reflection_map.bin').read_bytes())
  for y in range(32):
   for x in range(64):self.assertEqual(wet[y*64+x],(dry[(31-y)*64+x]&2047)|0x5000)
  for demo in ['water','split']:
   palettes=struct.unpack('>64H',(LAB/'res'/f'{demo}_palettes.bin').read_bytes());self.assertTrue(all(not v&~0xEEE for v in palettes))
   patterns=len((LAB/'res'/f'{demo}_ground.bin').read_bytes())//32
   self.assertLessEqual(0x2000+patterns*32,0x8000)
   mapping=struct.unpack('>2048H',(LAB/'res'/f'{demo}_map.bin').read_bytes());self.assertTrue(all(256<=v&2047<256+patterns for v in mapping))
  create=load(LAB.parents[1]/'scripts/create_project.py','raster_creator').create
  with tempfile.TemporaryDirectory() as d:
   for demo in ['water','split']:
    g=create(Path(d)/demo,demo=demo);self.assertIn('DEMO?='+demo.upper(),(g/'Makefile').read_text())
    with self.assertRaises(ValueError):create(g)
 def test_o0_o3_sanitized_long_sweep(self):
  harness=self.root/'stress.c';harness.write_text('''#include "raster.h"
#include <assert.h>
int main(void){RtWorld w;RtView v;for(unsigned d=0;d<2;d++){rt_init(&w,d);for(unsigned i=0;i<4000;i++){rt_advance(&w,i&1?5:6,i&15,i%193==0?32:i%251==0?64:0);rt_build(&w,&v);assert(rt_plan_valid(&v.plan));assert(v.count<=18);}}return 0;}''')
  for opt in ['-O0','-O3']:
   p=self.root/('stress'+opt[-1]+('.exe' if os.name=='nt' else ''))
   subprocess.run([self.cc,opt,*(['-fsanitize=undefined','-fno-sanitize-recover=all'] if os.name!='nt' else []),'-I'+str(LAB/'src'),str(harness),str(LAB/'src/raster.c'),'-o',str(p)],check=True,capture_output=True)
   subprocess.run([str(p)],check=True,capture_output=True)

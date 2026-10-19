"""Authoring validation, exact metatile transforms and scrolling cache regression."""
import ctypes as c
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-levels';LAB=SKILL/'assets/level-lab'
api=load(LAB/'tools/import_tiled.py','level_import')
class Area(c.Structure):_fields_=[('data',c.POINTER(c.c_ubyte)),('w',c.c_ushort),('h',c.c_ushort),('tiles',c.c_ushort),('spawns',c.c_ushort),('exits',c.c_ushort)]
class World(c.Structure):_fields_=[('areas',Area*4),('count',c.c_ushort)]
class Run(c.Structure):_fields_=[('address',c.c_ushort),('count',c.c_ushort),('step',c.c_ushort),('offset',c.c_ushort)]
class Patch(c.Structure):_fields_=[('runs',Run*4),('words',c.c_ushort*192),('count',c.c_ushort),('used',c.c_ushort)]
class LevelsTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
  self.root=Path(self.temp.name);self.res=self.root/'res';shutil.copytree(LAB/'res',self.res)
 def write_map(self,mutate):
  path=self.res/'orchard.tmj';m=api.read(path);mutate(m);path.write_text(json.dumps(m));return self.res/'world.json'
 def test_model_O0_O3_and_self_contained_generation(self):
  create=load(SKILL/'scripts/create_project.py','level_creator').create;game=create(self.root/'standalone game')
  with self.assertRaises(ValueError):create(game)
  result=subprocess.run([sys.executable,str(game/'tools/check_levels.py')],capture_output=True,text=True)
  self.assertEqual(result.returncode,0,result.stderr);self.assertTrue(json.loads(result.stdout)['passed'])
 def test_exact_prepared_art_and_source_freshness(self):
  files=api.compile_world(self.res/'world.json')
  for name,data in files.items():self.assertEqual(data,(self.res/name).read_bytes())
  self.assertTrue(api.verify(self.res/'world.json',self.res/'world.bin'))
  for name in ['world.bin','ground.bin','tileattrs.bin','palettes.bin']:
   p=self.res/name;old=p.read_bytes();p.write_bytes(old+b'!')
   with self.assertRaises(ValueError):api.verify(self.res/'world.json',self.res/'world.bin')
   p.write_bytes(old)
  self.write_map(lambda m:m['layers'][0]['data'].__setitem__(100,2))
  with self.assertRaises(ValueError):api.verify(self.res/'world.json',self.res/'world.bin')
 def test_build_art_step_never_overwrites_tiled_edits(self):
  game=load(SKILL/'scripts/create_project.py','level_create_edit').create(self.root/'game')
  paths=['orchard.tmj','archive.tmj','terrain.tsj','terrain.png','palettes.json']
  for name in paths:
   p=game/'res'/name;p.write_bytes(p.read_bytes()+b' ')
  before={name:(game/'res'/name).read_bytes() for name in paths}
  subprocess.run([sys.executable,str(game/'tools/make_art.py')],check=True,capture_output=True)
  self.assertEqual(before,{name:(game/'res'/name).read_bytes() for name in paths})
 def test_rejects_unsupported_layers_geometry_and_bounds(self):
  original=(self.res/'orchard.tmj').read_bytes()
  mutations=[lambda m:m.update(orientation='isometric'),lambda m:m.update(infinite=True),lambda m:m.update(width=97),
   lambda m:m['layers'][0].update(opacity=0.5),lambda m:m['layers'][0].update(offsetx=1),lambda m:m['layers'][0].update(encoding='base64'),
   lambda m:m['layers'][0]['data'].pop(),lambda m:m['layers'][0]['data'].__setitem__(0,0x20000001),lambda m:m['layers'][0]['data'].__setitem__(0,33),
   lambda m:m['layers'].append(dict(m['layers'][0])),lambda m:m['layers'][3]['objects'][0].update(x=0),
   lambda m:m['layers'][3]['objects'][0].update(rotation=20),lambda m:m['layers'][3]['objects'][5]['properties'][0].update(value='missing'),
   lambda m:m['layers'][3]['objects'][5]['properties'][1].update(value='missing')]
  for mutate in mutations:
   (self.res/'orchard.tmj').write_bytes(original)
   manifest=self.write_map(mutate)
   with self.subTest(mutate=mutate),self.assertRaises(ValueError):api.compile_world(manifest)
 def test_gid_mapping_flips_empty_and_stale_rotation_flag(self):
  self.assertEqual(api.decode(1,32),1);self.assertEqual(api.decode(0xC0000002,32),0xC002)
  self.assertEqual(api.decode(0x10000003,32),3)
  for value in [-1,True,0x100000000,0x20000001,0x80000000,33]:
   with self.assertRaises(ValueError):api.decode(value,32)
 def test_palette_alpha_and_local_paths_fail_explicitly(self):
  from PIL import Image
  path=self.res/'terrain.png'
  with Image.open(path) as source:im=source.convert('RGBA')
  for color in [(1,2,3,255),(255,255,255,128)]:
   im.putpixel((0,0),color);im.save(path)
   with self.assertRaises(ValueError):api.compile_world(self.res/'world.json')
  spec=api.read(self.res/'world.json');spec['areas'][0]['map']='../outside.tmj';(self.res/'world.json').write_text(json.dumps(spec))
  with self.assertRaises(ValueError):api.compile_world(self.res/'world.json')
 def test_nested_source_hash_paths_are_portable_and_output_never_replaces_sources(self):
  nested=self.res/'maps';nested.mkdir();path=nested/'orchard.tmj'
  m=api.read(self.res/'orchard.tmj');m['tilesets'][0]['source']='../terrain.tsj';path.write_text(json.dumps(m))
  spec=api.read(self.res/'world.json');spec['areas'][0]['map']='maps/orchard.tmj';(self.res/'world.json').write_text(json.dumps(spec))
  self.assertIn('maps/orchard.tmj',api.inputs(self.res/'world.json'))
  (self.res/'ground.bin').write_bytes((self.res/'palettes.json').read_bytes());spec['palette']='ground.bin';(self.res/'world.json').write_text(json.dumps(spec))
  before=(self.res/'ground.bin').read_bytes()
  with self.assertRaises(ValueError):api.write(self.res/'world.json',self.res/'world.bin')
  self.assertEqual(before,(self.res/'ground.bin').read_bytes())
 def test_shared_renderer_and_importer_have_one_implementation(self):
  self.assertEqual((SKILL/'scripts/import_tiled.py').read_bytes(),(LAB/'tools/import_tiled.py').read_bytes())
  for name in ['gfx.c','gfx.h','bridge.s']:
   self.assertEqual((LAB/'src'/name).read_bytes(),(ROOT/'skills/megadev-gameplay/assets/adventure-lab/src'/name).read_bytes())
 def test_cache_wraps_rows_columns_and_metatile_flips_against_independent_oracle(self):
  cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
  if not cc:self.skipTest('Host C compiler needed')
  dll=self.root/('levels.dll' if sys.platform=='win32' else 'levels.so')
  r=subprocess.run([cc,'-shared','-DLEVELS_HOST_DLL','-O2',*(['-fPIC'] if sys.platform!='win32' else []),'-I',str(LAB/'src'),str(LAB/'src/levels.c'),'-o',str(dll)],capture_output=True,text=True)
  self.assertEqual(r.returncode,0,r.stderr);lib=c.CDLL(str(dll))
  if sys.platform=='win32':
   import _ctypes
   self.addCleanup(_ctypes.FreeLibrary,lib._handle)
  lib.lv_open.restype=lib.lv_patch.restype=c.c_ushort
  lib.lv_open.argtypes=[c.POINTER(World),c.c_void_p,c.c_uint];lib.lv_patch.argtypes=[c.POINTER(Patch),c.POINTER(Area),c.c_void_p,*([c.c_ushort]*5)]
  m=api.read(self.res/'orchard.tmj');m['layers'][0]['data'][10*48+10]|=0xC0000000
  (self.res/'orchard.tmj').write_text(json.dumps(m));files=api.compile_world(self.res/'world.json');data=c.create_string_buffer(files['world.bin']);attrs=c.create_string_buffer(files['tileattrs.bin']);w=World()
  self.assertEqual(lib.lv_open(c.byref(w),data,len(data)-1),1)
  ground=m['layers'][0]['data'];foreground=m['layers'][1]['data'];attributes=struct.unpack('>32H',files['tileattrs.bin'])
  def expect(plane,x,y):
   code=(ground if plane==0 else foreground)[(y//2)*48+x//2] if x//2<48 and y//2<32 else 0
   plain=code&0x0fffffff
   if not plain:return 0
   qx,qy=x%2,y%2
   if code&0x80000000:qx^=1
   if code&0x40000000:qy^=1
   return 0x100+(plain-1)*4+qy*2+qx|attributes[plain-1]|(0x800 if code&0x80000000 else 0)|(0x1000 if code&0x40000000 else 0)
  memory={base+y*128+x*2:expect(plane,x,y) for plane,base in [(0,0xE000),(1,0xC000)] for y in range(32) for x in range(64)}
  ox=oy=0
  trajectory=[(min(448,i*8),min(320,i*8)) for i in range(57)]+[(448-i*8,max(0,320-i*8)) for i in range(57)]
  for x,y in trajectory:
   p=Patch();self.assertEqual(lib.lv_patch(c.byref(p),c.byref(w.areas[0]),attrs,ox,oy,x,y,0),1);self.assertLessEqual(p.count,4);self.assertLessEqual(p.used,192)
   for run in p.runs[:p.count]:
    for i in range(run.count):memory[run.address+i*run.step]=p.words[run.offset+i]
   for plane,base in [(0,0xE000),(1,0xC000)]:
    for wy in range(y//8,(y+191)//8+1):
     for wx in range(x//8,(x+319)//8+1):self.assertEqual(memory[base+(wy%32)*128+(wx%64)*2],expect(plane,wx,wy),(x,y,plane,wx,wy))
   ox,oy=x,y

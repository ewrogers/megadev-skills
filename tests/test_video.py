"""Cinema wire format, native pixels and bounded presentation contracts."""
import copy,ctypes as c,json,os,shutil,struct,subprocess,sys,tempfile,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-video/assets/cinema-lab'
movie=load(LAB/'tools/make_movie.py','movie_art')
creator=load(LAB.parent.parent/'scripts/create_project.py','movie_creator')
class State(c.Structure):_fields_=[(n,c.c_ushort) for n in ['frame','visible','uploaded','slot','clock','late','presented','done','paused']]
class VideoTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);cls.root=Path(cls.tmp.name)
  cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cc:
   if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':raise RuntimeError('Host C17 compiler required')
   raise unittest.SkipTest('Host C17 compiler required')
  lib=cls.root/('video.dll' if os.name=='nt' else 'video.so')
  subprocess.run([cc,'-std=c17','-O3','-shared',*(['-fPIC'] if os.name!='nt' else []),'-I',str(LAB/'src'),str(LAB/'src/cinema.c'),'-o',str(lib)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(lib));cls.lib.ci_header.argtypes=[c.c_void_p,c.c_uint,c.c_ushort];cls.lib.ci_header.restype=c.c_ushort
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(lambda:_ctypes.FreeLibrary(cls.lib._handle))
  for name in ['ci_init','ci_begin','ci_upload','ci_tick','ci_present','ci_due']:
   f=getattr(cls.lib,name);f.argtypes=[c.POINTER(State)]+([c.c_ushort] if name in ['ci_begin','ci_upload','ci_tick'] else []);f.restype=c.c_ushort if name not in ['ci_init','ci_tick'] else None
 def test_disc_groups_native_pixels_and_palette(self):
  root=self.root/'assets';root.mkdir();movie.generate(root)
  self.assertEqual(len(list((root/'disc').glob('*.CIN'))),6)
  for first in range(0,48,8):
   wire=(root/'disc'/f'C{first//8:02}.CIN').read_bytes();self.assertEqual(len(wire),63488)
   b=c.create_string_buffer(wire);self.assertTrue(self.lib.ci_header(b,len(wire),first))
   for i in range(8):
    raw=wire[2048+i*7680:2048+(i+1)*7680];self.assertEqual(raw,movie.frame(first+i));self.assertEqual(sum(struct.unpack(">3840H",raw))&65535,struct.unpack_from('>H',wire,32+i*2)[0])
  self.assertEqual(len({movie.frame(n) for n in range(48)}),48)
  colors=struct.unpack('>16H',(root/'res/palette.bin').read_bytes());self.assertTrue(all(v&~0xeee==0 for v in colors))
  self.assertEqual((root/'res/poster.bin').read_bytes(),movie.frame(0))
  for name in ['score','chime']:
   b=(root/'res'/f'{name}.pcm').read_bytes();self.assertNotIn(255,b if name=='score' else b[:-1])
 def test_header_rejects_malformed_dimensions_lengths_and_indices(self):
  root=self.root/'malformed';root.mkdir();movie.generate(root);raw=(root/'disc/C00.CIN').read_bytes()
  for at in list(range(20)):
   changed=bytearray(raw);changed[at]^=1
   self.assertFalse(self.lib.ci_header(c.create_string_buffer(bytes(changed)),len(raw),0),at)
  for size in [0,2048,63487,63489]:self.assertFalse(self.lib.ci_header(c.create_string_buffer(raw),size,0))
  for first in [1,3,4,48,65535]:self.assertFalse(self.lib.ci_header(c.create_string_buffer(raw),len(raw),first))
 def test_target_independent_timing_pause_and_late_holds(self):
  for hz,dt in [(60,5),(50,6)]:
   s=State();self.lib.ci_init(c.byref(s))
   for frame in range(48):
    self.assertTrue(self.lib.ci_begin(c.byref(s),frame))
    for size in [2048,2048,2048,1536]:self.assertTrue(self.lib.ci_upload(c.byref(s),size))
    if frame:
     for _ in range(hz//10):self.lib.ci_tick(c.byref(s),dt)
    self.assertTrue(self.lib.ci_present(c.byref(s)))
   self.assertEqual((s.presented,s.visible,s.done,s.late),(48,47,1,0))
  s=State();self.lib.ci_init(c.byref(s));s.paused=1;before=bytes(s);self.lib.ci_tick(c.byref(s),65535);self.assertEqual(bytes(s),before)
 def test_invalid_uploads_and_frame_order_leave_state_unchanged(self):
  s=State();self.lib.ci_init(c.byref(s));before=bytes(s)
  for size in [0,1,3,4098,65535]:self.assertFalse(self.lib.ci_upload(c.byref(s),size));self.assertEqual(bytes(s),before)
  self.assertFalse(self.lib.ci_begin(c.byref(s),48));self.assertEqual(bytes(s),before)
  self.assertFalse(self.lib.ci_present(c.byref(s)));self.assertEqual(bytes(s),before)
 def test_installed_portable_checks_and_creation_safeguards(self):
  p=creator.create(self.root/'standalone game',project_id='cinema')
  self.assertIn('PROJECT_ID:=cinema',(p/'Makefile').read_text())
  with self.assertRaises(ValueError):creator.create(p)
  with self.assertRaises(ValueError):creator.create(self.root/'bad','../bad')
  check=load(p/'tools/check_cinema.py','standalone_movie');self.assertIn('48 frames',check.check(p))

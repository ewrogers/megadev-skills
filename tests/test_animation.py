import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-animation';LAB=SKILL/'assets/animation-lab'
create=load(SKILL/'scripts/create_project.py','animation_create').create
class AnimationTests(unittest.TestCase):
 def test_timeline_and_consumer_contract_O0_O3(self):
  r=subprocess.run([sys.executable,str(LAB/'tools/check_animation.py')],capture_output=True,text=True)
  self.assertEqual(r.returncode,0,r.stderr);self.assertTrue(json.loads(r.stdout)['passed'])
 def test_standalone_native_art_and_hitbox_guides(self):
  with tempfile.TemporaryDirectory() as folder:
   game=create(Path(folder)/'animation game')
   with self.assertRaises(ValueError):create(game)
   module=load(game/'tools/make_art.py','animation_art');art=module.build()
   self.assertEqual(module.vdp(7,0,0),0x00E);self.assertEqual(module.vdp(0,7,0),0x0E0);self.assertEqual(module.vdp(0,0,7),0xE00)
   with self.assertRaises(ValueError):module.vdp(8,0,0)
   for name,data in art.items():self.assertEqual(data,(game/'res'/name).read_bytes())
   self.assertEqual(len(art['actors.bin']),190*32)
   self.assertTrue(all(not(c&~0xEEE) for c in struct.unpack('>64H',art['palettes.bin'])))
   for w,h,color in [(20,14,13),(26,14,13),(24,14,13),(10,16,8),(14,20,6)]:
    p=module.box(w,h,color)
    self.assertTrue(all(p[y][x]==(color if x<w and y<h and (x==0 or x==w-1 or y==0 or y==h-1) else 0) for y in range(32) for x in range(32)))
 def test_reuses_graphics_and_interrupt_contract(self):
  for name in ['gfx.c','gfx.h','bridge.s']:
   self.assertEqual((LAB/'src'/name).read_bytes(),(ROOT/'skills/megadev-gameplay/assets/adventure-lab/src'/name).read_bytes())

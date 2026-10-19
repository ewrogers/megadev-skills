import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-gameplay';LAB=SKILL/'assets/adventure-lab'
create=load(SKILL/'scripts/create_project.py','gameplay_create').create
class GameplayTests(unittest.TestCase):
 def test_gameplay_contract_O0_O3(self):
  result=subprocess.run([sys.executable,str(LAB/'tools/check_gameplay.py')],capture_output=True,text=True)
  self.assertEqual(result.returncode,0,result.stderr);self.assertTrue(json.loads(result.stdout)['passed'])
 def test_standalone_and_native_art(self):
  with tempfile.TemporaryDirectory() as d:
   game=create(Path(d)/'adventure game')
   with self.assertRaises(ValueError):create(game)
   art=load(game/'tools/make_art.py','adventure_art').build()
   for name,data in art.items():self.assertEqual(data,(game/'res'/name).read_bytes())
   self.assertEqual(len(art['actors.bin']),92*32);self.assertEqual(len(art['ground.bin']),10*32)
   self.assertTrue(all(not (c&~0xEEE) for c in struct.unpack('>64H',art['palettes.bin'])))
 def test_reused_coordination_and_sprite_owners(self):
  for name in ['coord.c','coord.h','main_link.c','sub_link.c','spx.c','bridge.s']:
   self.assertEqual((LAB/'src'/name).read_bytes(),(ROOT/'skills/megadev-system/assets/system-lab/src'/name).read_bytes())
  for name in ['gfx.c','gfx.h']:
   self.assertEqual((LAB/'src'/name).read_bytes(),(ROOT/'skills/megadev-graphics/assets/graphics-lab/src'/name).read_bytes())

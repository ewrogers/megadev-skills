"""Original-game integration contracts, authoring limits and owner provenance."""
import ast
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
GAME=ROOT/'validation/glasshouse'
class CrossSkillTests(unittest.TestCase):
 def test_original_game_and_save_models_at_O0_O3(self):
  r=subprocess.run([sys.executable,str(GAME/'tools/check_garden.py')],capture_output=True,text=True)
  self.assertEqual(r.returncode,0,r.stderr)
 def test_original_generator_refuses_overwrite_and_installed_game_is_independent(self):
  create=load(ROOT/'validation/create_project.py','validation_creator').create
  with tempfile.TemporaryDirectory() as d:
   dest=create(Path(d)/'original game')
   with self.assertRaises(ValueError):create(dest)
   self.assertIn('PROJECT_ID:=glasshouse',(dest/'Makefile').read_text())
   r=subprocess.run([sys.executable,str(dest/'tools/check_garden.py')],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
 def test_ascii_font_has_real_glyphs_for_both_cases_and_authored_captions_fit(self):
  font=load(GAME/'tools/make_font.py','glass_font').build()
  self.assertEqual(font,(GAME/'res/font.bin').read_bytes())
  blank=font[:32]
  for char in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,?[]':
   data=font[(ord(char)-32)*32:(ord(char)-31)*32];self.assertNotEqual(data,blank,char)
  source=(GAME/'src/garden.c').read_text()
  for m in re.finditer(r'txt\(c,(\d+),(\d+),(\d+),([^;]*?)\);',source):
   self.assertLessEqual(int(m[1])+int(m[3]),40)
   for q in re.findall(r'"(?:[^"\\]|\\.)*"',m[4]):self.assertLessEqual(len(ast.literal_eval(q)),int(m[3]),q)
 def test_prepared_art_is_original_native_and_within_each_budget(self):
  art=load(GAME/'tools/make_art.py','glass_art');verify=load(GAME/'tools/verify_art.py','glass_verify')
  for name,value in [('garden',art.picture(0)),('cavern',art.picture(1)),('actor',art.actor())]:
   self.assertEqual(value,(GAME/'res'/(name+'.mda')).read_bytes());self.assertGreater(verify.validate(GAME/'res'/(name+'.mda'),name=='actor'),64)
  self.assertLessEqual(len(art.panels())//32+0xB0,0xC0)
 def test_mechanical_service_owners_are_reused_without_copying_old_game(self):
  base=ROOT/'skills/megadev-best-practices/assets/harbor-relay/src'
  for name in ['stream.c','stream.h','stream_main.c','stream_sub.c','audio.c','audio.h','audio_main.c','audio_sub.c','scene.c','scene.h','checkpoint_main.c','checkpoint_sub.c']:
   self.assertEqual((GAME/'src'/name).read_bytes(),(base/name).read_bytes(),name)
  for name in ['ui.c','paint.c']:
   self.assertEqual((GAME/'src'/name).read_bytes(),(ROOT/'skills/megadev-dialogue/assets/dialogue-lab/src'/name).read_bytes())
  text=(GAME/'src/garden.c').read_text();self.assertNotIn('HARBOR RELAY',text);self.assertNotIn('LANTERN QUAY',text)

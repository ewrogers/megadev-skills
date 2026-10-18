import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-scenes'
LAB=SKILL/'assets/transition-lab'
class SceneTests(unittest.TestCase):
    def test_beacon_is_transparent_native_sprite_with_white_outline(self):
        tool=load(LAB/'tools/make_font.py','scene_art')
        data=tool.beacon();self.assertEqual(len(data),128)
        self.assertEqual(data,(LAB/'res/beacon.bin').read_bytes())
        pixels=[[0]*16 for _ in range(16)]
        for tx in range(2):
            for ty in range(2):
                tile=data[(tx*2+ty)*32:(tx*2+ty+1)*32]
                for y in range(8):
                    for x in range(8):pixels[ty*8+y][tx*8+x]=(tile[y*4+x//2]>>(0 if x&1 else 4))&15
        self.assertEqual({v for row in pixels for v in row},{0,1,3})
        self.assertEqual(pixels[0][0],0);self.assertEqual(pixels[7][7],3)
        self.assertEqual(pixels[0][7],1);self.assertEqual(pixels[7][0],1)

    def test_transitions_leases_failure_and_latest_request_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('scenes'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/scene.c'),str(ROOT/'tests/fixtures/scenes.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_standalone_generation_preserves_existing_project(self):
        create=load(SKILL/'scripts/create_project.py','scenes_creator').create
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'transition game')
            self.assertTrue((game/'src/scene.c').is_file())
            with self.assertRaises(ValueError):create(game)
            with self.assertRaises(ValueError):create(Path(folder)/'bad',project_id='../bad')

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-scaling'
LAB=SKILL/'assets/scaling-lab'
class ScalingTests(unittest.TestCase):
    def test_trace_bounds_identity_rotations_and_projection_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('scaling'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/scale.c'),str(ROOT/'tests/fixtures/scaling.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_independent_generation_never_replaces_project(self):
        create=load(SKILL/'scripts/create_project.py','scaling_creator').create
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'scaling game')
            self.assertTrue((game/'src/scale_sub.c').is_file())
            with self.assertRaises(ValueError):create(game)

    def test_art_rebuild_native_stamps_palette_and_identity_crop(self):
        import struct
        maker=load(LAB/'tools/make_art.py','scaling_art')
        pixels=maker.art();self.assertEqual((len(pixels),len(pixels[0])),(256,256))
        self.assertEqual(maker.stamp_bytes(pixels),(LAB/'res/stamps.bin').read_bytes())
        self.assertEqual(maker.png(pixels),(LAB/'res/observatory.png').read_bytes())
        self.assertEqual(struct.pack('>16H',*maker.COLORS),(LAB/'res/palette.bin').read_bytes())
        self.assertTrue(all((c&~0xEEE)==0 for c in maker.COLORS))
        data=maker.stamp_bytes(pixels);self.assertEqual(len(data),32896)
        self.assertEqual(data[:128],bytes(128))
        for y in range(256):
            for x in range(256):
                stamp=1+(y//16)*16+x//16
                offset=stamp*128+(x%16//8)*64+(y%16)*4+(x%8)//2
                self.assertEqual((data[offset]>>(0 if x&1 else 4))&15,pixels[y][x])

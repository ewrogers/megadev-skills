import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-streaming'
LAB=SKILL/'assets/streaming-lab'
create=load(SKILL/'scripts/create_project.py','stream_creator').create
class StreamingTests(unittest.TestCase):
    def test_sector_bounds_and_incremental_decoder_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('stream'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/stream.c'),str(ROOT/'tests/fixtures/streaming.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                result=subprocess.run(args,capture_output=True,text=True);self.assertEqual(result.returncode,0,result.stderr)
                result=subprocess.run([str(binary),str(LAB/'res/garden.mda'),str(LAB/'res/cavern.mda')],capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stderr)
    def test_standalone_generator_preserves_existing_project_and_font(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'stream game')
            self.assertTrue((game/'src/stream_sub.c').is_file())
            with self.assertRaises(ValueError):create(game)
            font=load(game/'tools/make_font.py','stream_font')
            self.assertEqual(font.build(),(game/'res/font.bin').read_bytes())
            for name in ['garden','cavern','actor']:
                self.assertEqual((game/'res'/f'{name}.mda').read_bytes(),(LAB/'res'/f'{name}.mda').read_bytes())

    def test_art_preflight_rejects_corruption_and_emits_sizes(self):
        tool=load(LAB/'tools/verify_art.py','stream_art')
        sizes=tool.verify(LAB)
        self.assertEqual(sizes,{'garden':15680,'cavern':21216,'actor':448})
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'bad.mda'
            data=bytearray((LAB/'res/garden.mda').read_bytes());data[100]^=1;path.write_bytes(data)
            with self.assertRaises(ValueError):tool.validate(path)
            path.write_bytes((LAB/'res/actor.mda').read_bytes())
            with self.assertRaises(ValueError):tool.validate(path)

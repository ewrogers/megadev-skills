import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-input'
LAB=SKILL/'assets/input-lab'
create=load(SKILL/'scripts/create_project.py','input_creator').create
class InputTests(unittest.TestCase):
    def test_input_state_and_protocol_contracts_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for fixture in ['input','protocol']:
                for mode in ['-O0','-O3']:
                    binary=Path(folder)/(fixture+mode+('.exe' if os.name=='nt' else ''))
                    args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/input.c'),str(LAB/'src/protocol.c'),str(ROOT/'tests/fixtures'/(fixture+'.c')),'-o',str(binary)]
                    if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                    r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                    r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_installed_style_generator_preserves_existing_destination(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'input game')
            self.assertTrue((game/'src/protocol.c').is_file())
            self.assertTrue((game/'src/platform.c').is_file())
            with self.assertRaises(ValueError):create(game)
            font=load(game/'tools/make_font.py','input_font')
            self.assertEqual(font.build(),(game/'res/font.bin').read_bytes())

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-system'
LAB=SKILL/'assets/system-lab'
create=load(SKILL/'scripts/create_project.py','system_creator').create
class SystemTests(unittest.TestCase):
    def test_command_and_buffer_lifetimes_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('coord'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/coord.c'),str(ROOT/'tests/fixtures/coord.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_independent_generator_and_font(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'system game')
            self.assertTrue((game/'src/sub_link.c').is_file())
            with self.assertRaises(ValueError):create(game)
            font=load(game/'tools/make_font.py','system_font')
            self.assertEqual(font.build(),(game/'res/font.bin').read_bytes())

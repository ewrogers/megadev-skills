import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-hardware'
create=load(SKILL/'scripts/create_project.py','hardware_creator').create
class HardwareTests(unittest.TestCase):
    def test_installed_creator_and_existing_files(self):
        with tempfile.TemporaryDirectory(prefix='hardware skill ') as folder:
            folder=Path(folder);installed=folder/'installed'
            shutil.copytree(SKILL,installed)
            r=subprocess.run([sys.executable,str(installed/'scripts/create_project.py'),str(folder/'game')],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            self.assertIn('PROJECT_ID:=hardware',(folder/'game/Makefile').read_text())
            self.assertTrue((folder/'game/src/probe.c').is_file())
            original=(folder/'game/src/ipx.c').read_bytes()
            with self.assertRaises(ValueError):create(folder/'game')
            self.assertEqual((folder/'game/src/ipx.c').read_bytes(),original)
    def test_probe_patterns_corruption_zero_length_and_guard_bytes(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('C compiler unavailable')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                out=Path(folder)/('probe'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(SKILL/'assets/hardware-lab/src'),str(SKILL/'assets/hardware-lab/src/probe.c'),str(ROOT/'tests/fixtures/probe.c'),'-o',str(out)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,text=True,capture_output=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(out)],text=True,capture_output=True);self.assertEqual(r.returncode,0,r.stderr)

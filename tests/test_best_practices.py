import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-best-practices'
LAB=SKILL/'assets/harbor-relay'

class BestPracticesTests(unittest.TestCase):
    def test_standalone_creation_and_existing_owner_contracts(self):
        with tempfile.TemporaryDirectory() as folder:
            installed=Path(folder)/'installed';shutil.copytree(SKILL,installed)
            create=load(installed/'scripts/create_project.py','practice_creator').create
            game=create(Path(folder)/'relay game')
            self.assertTrue((game/'src/checkpoint_sub.c').is_file())
            self.assertIn('PROJECT_ID:=harbor',(game/'Makefile').read_text())
            with self.assertRaises(ValueError):create(game)
            for file in ['scene.c','scene.h','stream.c','stream.h','stream_main.c','stream_sub.c','audio.c','audio.h','audio_main.c','audio_sub.c']:
                self.assertEqual((game/'src'/file).read_bytes(),(ROOT/'skills/megadev-scenes/assets/transition-lab/src'/file).read_bytes(),file)
            self.assertEqual((game/'src/save.c').read_bytes(),(ROOT/'skills/megadev-save/assets/save-lab/src/save.c').read_bytes())
            self.assertEqual((game/'src/bram.s').read_text().replace('MDBESTLAB_','MDSAVELAB_'),(ROOT/'skills/megadev-save/assets/save-lab/src/bram.s').read_text())
            self.assertEqual((game/'tools/release.py').read_bytes(),(ROOT/'skills/megadev-release/scripts/release.py').read_bytes())

    def test_c_contracts_at_O0_O3_and_undefined_behavior_sanitizer(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C17 compiler required')
            self.skipTest('C17 compiler unavailable')
        with tempfile.TemporaryDirectory() as folder:
            for opt in ['-O0','-O3']:
                exe=Path(folder)/('contracts'+opt+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/contracts.c'),str(LAB/'tools/contracts_checks.c'),'-o',str(exe)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                subprocess.run(args,check=True,capture_output=True)
                subprocess.run([str(exe)],check=True,capture_output=True)

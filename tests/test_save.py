import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-save'
LAB=SKILL/'assets/save-lab'
class SaveTests(unittest.TestCase):
    def test_records_recovery_migration_failure_and_write_preservation_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('save'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/save.c'),str(ROOT/'tests/fixtures/save.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_independent_generation_never_replaces_project(self):
        create=load(SKILL/'scripts/create_project.py','save_creator').create
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'save game')
            self.assertTrue((game/'src/bram.s').is_file())
            with self.assertRaises(ValueError):create(game)

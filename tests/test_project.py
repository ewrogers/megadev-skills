import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SKILL=ROOT/'skills/megadev-project'
LAB=SKILL/'assets/project-lab'
def load(path,name):
    spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
create=load(SKILL/'scripts/create_project.py','creator').create
class ProjectTests(unittest.TestCase):
    def test_standalone_creation_paths_and_preserving_existing_files(self):
        with tempfile.TemporaryDirectory(prefix='megadev project ') as folder:
            scratch=Path(folder);out=create(scratch/'my game',title='My First Game',project_id='my_game')
            self.assertIn('PROJECT_ID:=my_game',(out/'Makefile').read_text())
            self.assertTrue((out/'src/ip.s').is_file());self.assertTrue((out/'LICENSE.megadev').is_file())
            original=(out/'Makefile').read_bytes()
            with self.assertRaises(ValueError):create(out)
            self.assertEqual((out/'Makefile').read_bytes(),original)
            copied=scratch/'installed skill';shutil.copytree(SKILL,copied)
            r=subprocess.run([sys.executable,str(copied/'scripts/create_project.py'),str(scratch/'another game')],capture_output=True,text=True)
            self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual((scratch/'another game/src/ipx.c').read_bytes(),(LAB/'src/ipx.c').read_bytes())
    def test_names_cannot_inject_make_commands_or_escape_paths(self):
        with tempfile.TemporaryDirectory() as scratch:
            for name in ['../game','a'*12,'$(shell id)','Game','a/b','']:
                with self.subTest(name=name),self.assertRaises(ValueError):create(Path(scratch)/'out',project_id=name)
            for title in ['$(shell id)','x"\nall:','x#comment','é','']:
                with self.subTest(title=title),self.assertRaises(ValueError):create(Path(scratch)/'out',title=title)
            self.assertFalse((Path(scratch)/'out').exists())
        with self.assertRaises(ValueError):create(LAB/'src/forbidden')
    def test_control_edges_bounds_and_command_lifetimes(self):
        cc=next((p for name in ['clang','cc','gcc'] if (p:=shutil.which(name))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C17 compiler required')
            self.skipTest('C17 compiler required')
        with tempfile.TemporaryDirectory() as scratch:
            for mode in ['-O0','-O3']:
                output=Path(scratch)/('model'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/model.c'),str(ROOT/'tests/fixtures/model.c'),'-o',str(output)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(output)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
if __name__=='__main__':unittest.main()

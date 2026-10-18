import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-debug'
LAB=SKILL/'assets/debug-lab'
create=load(SKILL/'scripts/create_project.py','debug_creator').create
lookup=load(SKILL/'scripts/lookup_address.py','debug_lookup')
class DebugTests(unittest.TestCase):
    def test_trace_wrap_freeze_clear_and_bounds(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('trace'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/trace.c'),str(ROOT/'tests/fixtures/trace.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                subprocess.run(args,check=True,capture_output=True);subprocess.run([str(binary)],check=True,capture_output=True)
    def test_identity_and_code_bounds_before_symbol_lookup(self):
        with tempfile.TemporaryDirectory() as folder:
            elf=Path(folder)/'ipx.mmd.elf';elf.write_bytes(b'identity fixture')
            digest=hashlib.sha256(elf.read_bytes()).hexdigest()
            with patch.object(lookup.subprocess,'run') as run:
                for cpu,module,pc,sha in [('sub','main',0xff0100,digest),('main','room',0x200100,digest),('main','main',0xff0101,digest),('main','main',0xff0100,'bad')]:
                    with self.assertRaises(ValueError):lookup.resolve(elf,cpu,module,pc,sha)
                run.assert_not_called()
                run.return_value.stdout='00ff0100 A _TEXT_ORIGIN\n00000020 A _TEXT_LENGTH\n'
                with self.assertRaises(ValueError):lookup.resolve(elf,'main','main',0xff0200,digest)
                run.side_effect=[subprocess.CompletedProcess([],0,stdout=run.return_value.stdout),subprocess.CompletedProcess([],0,stdout='entry\n??:0\n')]
                self.assertFalse(lookup.resolve(elf,'main','main',0xff0100,digest)['source_available'])
    def test_standalone_and_owned_coordination_seam(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'debug game')
            with self.assertRaises(ValueError):create(game)
            self.assertEqual((game/'src/coord.c').read_bytes(),(ROOT/'skills/megadev-system/assets/system-lab/src/coord.c').read_bytes())
            self.assertEqual((game/'tools/lookup_address.py').read_bytes(),(SKILL/'scripts/lookup_address.py').read_bytes())
            self.assertEqual(load(game/'tools/make_font.py','debug_font').build(),(game/'res/font.bin').read_bytes())

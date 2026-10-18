import os
import copy
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-memory'
LAB=SKILL/'assets/memory-lab'
create=load(SKILL/'scripts/create_project.py','memory_creator').create
class MemoryTests(unittest.TestCase):
    def test_linked_layout_exact_fits_overlap_and_embedded_accounting(self):
        tool=load(LAB/'tools/report_layout.py','layout_tool')
        config={'pools':[{'kind':'main','name':'Main','start':0xFF0000,'bytes':65536,'regions':[
            {'name':'code','start':0xFF0000,'bytes':4096,'module':'x','section':'rom'},
            {'name':'state','start':0xFFA000,'bytes':128,'module':'x','section':'ram'}]}],
            'embedded':[{'name':'arena','symbol':'scene_pool','bytes':64,'module':'x','region':'state'}],
            'word_bank_payload_bytes':131072}
        values={'MODULE_ROM_ORIGIN':0xFF0000,'MODULE_ROM_LENGTH':4096,'_ROM_DATA_ORIGIN':0xFF0FF0,
                'MODULE_RAM_ORIGIN':0xFFA000,'MODULE_RAM_LENGTH':128,'_RAM_DATA_ORIGIN':0xFFA070,
                '_RAM_DATA_LENGTH':16,'scene_pool':0xFFA020}
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);(root/'build').mkdir();path=root/'build/x.elf.sym'
            path.write_text(''.join(f'{v:08x} A {k}\n' for k,v in values.items()))
            report=tool.report(root,config)
            self.assertEqual(report['pools'][0]['reserved'],4224)
            self.assertEqual(report['embedded'],[{'name':'arena','bytes':64,'already_counted':True}])
            mutations=[lambda q:q['pools'][0].update(bytes=65537),
                       lambda q:q['pools'].append(copy.deepcopy(q['pools'][0])),
                       lambda q:q['pools'][0]['regions'][1].update(start=0xFF0800),
                       lambda q:q['pools'][0]['regions'][1].update(start=0xFFFFF0),
                       lambda q:q['embedded'][0].update(bytes=1024),
                       lambda q:q.update(word_bank_payload_bytes=131073)]
            for mutate in mutations:
                bad=copy.deepcopy(config);mutate(bad)
                with self.assertRaises(ValueError):tool.report(root,bad)
            values['_ROM_DATA_ORIGIN']+=1
            path.write_text(''.join(f'{v:08x} A {k}\n' for k,v in values.items()))
            with self.assertRaises(ValueError):tool.report(root,config)
    def test_allocation_generations_leases_and_bounds_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('memory'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/memory.c'),str(ROOT/'tests/fixtures/memory.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_independent_creation_and_existing_file_preservation(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'memory game')
            self.assertTrue((game/'src/memory_sub.c').is_file())
            with self.assertRaises(ValueError):create(game)
            font=load(game/'tools/make_font.py','memory_font')
            self.assertEqual(font.build(),(game/'res/font.bin').read_bytes())

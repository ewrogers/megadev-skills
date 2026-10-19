import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from test_project import ROOT,load

SKILL=ROOT/'skills/megadev-ui'
LAB=SKILL/'assets/menu-lab'
create=load(SKILL/'scripts/create_project.py','ui_creator').create

class UiTests(unittest.TestCase):
    def test_reports_reject_missing_ui_execution_and_incomplete_regions(self):
        import copy
        import json
        from compare_reports import compare,CASES
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},
              'startup':{'main':{'probe_execution':True,'ui_execution':True},'sub':{},'room':{}},'sub_callback':True}
        a={'passed':True,'skill':'megadev-ui','platform':'linux/arm64','cases':{key:copy.deepcopy(case) for key in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for path,report in zip(paths,(a,right)):path.write_text(json.dumps(report))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in (lambda r:r['cases'].pop('eu-release'),
                           lambda r:r['cases'].pop('us-tour'),
                           lambda r:r['cases']['us-release']['startup']['main'].pop('ui_execution'),
                           lambda r:r['cases']['us-debug']['startup']['main'].pop('probe_execution')):
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

    def test_ui_contracts_O0_O3(self):
        if not any(shutil.which(n) for n in ('cc','clang','gcc')):
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host compiler')
        api=load(LAB/'tools/check_ui.py','ui_check')
        self.assertTrue(api.check()['passed'])

    def test_standalone_assets_repeat_and_fit_reserved_vram(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'menu game')
            with self.assertRaises(ValueError):create(game)
            expected={p.name:p.read_bytes() for p in (game/'res').glob('*.bin')}
            result=subprocess.run([sys.executable,str(game/'tools/make_assets.py')],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(expected,{p.name:p.read_bytes() for p in (game/'res').glob('*.bin')})
            counts={'fonts':192,'heading':144,'panels':13,'icons':32,'portrait':36,'backdrop':4}
            for name,count in counts.items():self.assertEqual(len(expected[name+'.bin']),count*32)
            palette=struct.unpack('>64H',expected['palette.bin'])
            self.assertTrue(all(v&~0xEEE==0 for v in palette))
            self.assertEqual((0x1C8*32)<0x9000,True)
            self.assertEqual(len(palette),64)

    def test_host_preview_renders_all_pages(self):
        if not any(shutil.which(n) for n in ('cc','clang','gcc')):self.skipTest('No host compiler')
        with tempfile.TemporaryDirectory() as folder:
            api=load(LAB/'tools/preview.py','ui_preview');out=Path(folder)/'screens';api.preview(out)
            self.assertEqual(len(list(out.glob('*.png'))),8)
            from PIL import Image
            for p in out.glob('*.png'):
                with Image.open(p) as image:
                    self.assertEqual(image.size,(320,224));self.assertGreater(len(image.getcolors(1000)),2)

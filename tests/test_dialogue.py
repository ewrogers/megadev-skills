import copy
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import wave
from test_project import ROOT,load

SKILL=ROOT/'skills/megadev-dialogue'
LAB=SKILL/'assets/dialogue-lab'
create=load(SKILL/'scripts/create_project.py','dialogue_creator').create

class DialogueTests(unittest.TestCase):
    def test_executable_dialogue_and_voice_policy_O0_O3(self):
        if not any(shutil.which(n) for n in ('cc','clang','gcc')):
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('Host C compiler required')
            self.skipTest('No host compiler')
        self.assertTrue(load(LAB/'tools/check_dialogue.py','dialogue_check').check()['passed'])

    def test_installed_project_art_and_resident_samples_are_reproducible(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'dialogue game')
            with self.assertRaises(ValueError):create(game)
            for name in ('audio.c','audio_bios.s'):
                self.assertEqual((game/'src'/name).read_bytes(),(ROOT/'skills/megadev-audio/assets/audio-lab/src'/name).read_bytes())
            painter=(game/'src/paint.c').read_text().replace('screen==0xFFFFu','screen>=UI_PAGES*2u')
            self.assertEqual(painter,(ROOT/'skills/megadev-ui/assets/menu-lab/src/paint.c').read_text())
            before={p.name:p.read_bytes() for p in (game/'res').iterdir() if p.is_file()}
            for tool in ('make_assets.py','make_audio.py'):
                r=subprocess.run([sys.executable,str(game/'tools'/tool)],capture_output=True,text=True)
                self.assertEqual(r.returncode,0,r.stderr)
            self.assertEqual(before,{p.name:p.read_bytes() for p in (game/'res').iterdir() if p.is_file()})
            self.assertEqual(len((game/'res/portrait.bin').read_bytes()),216*32)
            for name in ('welcome','listen'):
                with wave.open(str(game/'res'/f'voice_{name}.wav')) as wav:
                    self.assertEqual((wav.getnchannels(),wav.getsampwidth(),wav.getframerate()),(1,2,8000))
                data=(game/'res'/f'voice_{name}.pcm').read_bytes()
                self.assertTrue(1<=len(data)<=12000);self.assertNotIn(255,data)

    def test_voice_budget_rejects_oversize_and_stereo_without_truncation(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'game')
            tool=load(game/'tools/make_audio.py','bounded_voice_encoder')
            for channels,frames in ((2,8000),(1,12001)):
                path=game/'res/voice_welcome.wav'
                with wave.open(str(path),'wb') as wav:
                    wav.setparams((channels,2,8000,0,'NONE','not compressed'));wav.writeframes(bytes(frames*channels*2))
                source=path.read_bytes()
                with self.assertRaises(ValueError):tool.generate(game)
                self.assertEqual(path.read_bytes(),source)

    def test_native_preview_covers_branches_paired_panels_and_styles(self):
        if not any(shutil.which(n) for n in ('cc','clang','gcc')):self.skipTest('No host compiler')
        with tempfile.TemporaryDirectory() as folder:
            output=Path(folder)/'screens';load(LAB/'tools/preview.py','dialogue_preview').preview(output)
            self.assertEqual(len(list(output.glob('*.png'))),13)
            from PIL import Image
            for path in output.glob('*.png'):
                with Image.open(path) as image:self.assertEqual(image.size,(320,224))

    def test_report_requires_both_voice_endpoints_and_full_matrix(self):
        from compare_reports import compare,CASES
        case={'disc':{'files':{'IPX.MMD':'hash'}},'boot_payloads':{'sp.bin':'hash'},'sub_callback':True,
              'startup':{'main':{'dialogue_execution':True,'voice_execution':True},'sub':{'voice_execution':True},'room':{}}}
        a={'passed':True,'skill':'megadev-dialogue','platform':'linux/arm64','cases':{k:copy.deepcopy(case) for k in CASES|{'us-tour'}}}
        b=copy.deepcopy(a);b['platform']='linux/amd64'
        with tempfile.TemporaryDirectory() as folder:
            paths=[Path(folder)/'a.json',Path(folder)/'b.json']
            def run(right):
                for path,report in zip(paths,(a,right)):path.write_text(json.dumps(report))
                return compare(paths)
            self.assertTrue(run(b))
            for mutate in (lambda r:r['cases'].pop('us-tour'),lambda r:r['cases'].pop('eu-release'),
                           lambda r:r['cases']['us-debug']['startup']['main'].pop('dialogue_execution'),
                           lambda r:r['cases']['us-release']['startup']['sub'].pop('voice_execution')):
                bad=copy.deepcopy(b);mutate(bad)
                with self.assertRaises(ValueError):run(bad)

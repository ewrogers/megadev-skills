import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-testing';LAB=SKILL/'assets/test-lab'
create=load(SKILL/'scripts/create_project.py','testing_creator').create
verify=load(LAB/'tools/verify_replay.py','replay_verifier').verify
class TestingTests(unittest.TestCase):
    def test_replay_region_clock_events_overflow_and_invalid_tapes(self):
        cc=next((p for n in ['cc','clang','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        for optimization in ['-O0','-O3']:
            result=verify(LAB,cc,optimization)
            self.assertEqual(result.returncode,0,result.stderr)
            report=json.loads(result.stdout);self.assertTrue(report['passed'])
            self.assertEqual(report['final_hash'],'a61a03c9')
        result=verify(LAB,cc,inject=True)
        self.assertEqual(result.returncode,1,result.stderr)
        report=json.loads(result.stdout)
        self.assertEqual(report['first_divergent_tick'],0)
        self.assertNotEqual(report['expected_hash'],report['actual_hash'])
    def test_standalone_generator_and_service_ownership(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'testing game')
            with self.assertRaises(ValueError):create(game)
            for name in ['stream.c','stream_main.c','stream_sub.c','audio.c','audio_main.c','audio_sub.c']:
                self.assertEqual((game/'src'/name).read_bytes(),(ROOT/'skills/megadev-audio/assets/audio-lab/src'/name).read_bytes())
            self.assertTrue((game/'tools/verify_replay.py').is_file())
            self.assertTrue((game/'tools/make_audio.py').is_file())

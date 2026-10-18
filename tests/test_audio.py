import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import wave
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-audio'
LAB=SKILL/'assets/audio-lab'
create=load(SKILL/'scripts/create_project.py','audio_creator').create
class AudioTests(unittest.TestCase):
    def test_voice_priorities_lifetimes_and_drive_ownership_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('audio'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/audio.c'),str(ROOT/'tests/fixtures/audio.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_pcm_encoding_loop_bounds_and_pitch_steps(self):
        tool=load(LAB/'tools/make_audio.py','audio_gen')
        self.assertEqual(tool.pcm_encode(0),0x80)
        self.assertEqual(tool.pcm_encode(1),0xfe)
        self.assertEqual(tool.pcm_encode(-1),0x7e)
        for name in ['bed','bell','pluck','drum']:
            data,rate=tool.pcm_sample(name)
            self.assertNotIn(0xff,data)
            self.assertLess(len(data)+1,4096)
            self.assertGreater(len(set(data)),40)
            step=(rate*2048*384+6250000)//12500000
            self.assertLess(abs(step*12500000/(384*2048)-rate),9)
            if name!='bed':self.assertEqual(data[-1],0x80)
    def test_mixed_mode_layout_rejects_bad_audio_and_preserves_existing_cue(self):
        cue=load(LAB/'tools/make_cue.py','audio_cue')
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);(root/'res').mkdir();image=root/'my game.iso';image.write_bytes(b'SEGADISCSYSTEM  '.ljust(2048,b'\0'))
            def audio(rate=44100,pregap=True):
                for track in [2,3]:
                    with wave.open(str(root/'res'/f'track{track:02}.wav'),'wb') as out:
                        out.setparams((2,2,rate,0,'NONE','not compressed'));out.writeframes(bytes(44100*6*4) if pregap else b'\1'*(44100*6*4))
            audio();path=cue.write_cue(image);text=path.read_text()
            self.assertIn('TRACK 02 AUDIO',text);self.assertIn('INDEX 01 00:02:00',text)
            self.assertEqual(cue.write_cue(image).read_text(),text)
            path.write_text('user mixed-mode layout')
            with self.assertRaises(FileExistsError):cue.write_cue(image)
            self.assertEqual(path.read_text(),'user mixed-mode layout');path.unlink()
            audio(22050)
            with self.assertRaises(ValueError):cue.write_cue(image)
            audio(pregap=False)
            with self.assertRaises(ValueError):cue.write_cue(image)
    def test_independent_generator_and_music_generation(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'audio game')
            with self.assertRaises(ValueError):create(game)
            tool=load(game/'tools/make_audio.py','generated_audio');tool.generate(game)
            stamp=(game/'build/audio-source.json').read_bytes();tool.generate(game)
            self.assertEqual(stamp,(game/'build/audio-source.json').read_bytes())
            header=(game/'build/audio_assets.h').read_bytes()
            (game/'build/audio_assets.h').write_text('corrupt constants')
            tool.generate(game)
            self.assertEqual(header,(game/'build/audio_assets.h').read_bytes())
            for track in [2,3]:
                with wave.open(str(game/'res'/f'track{track:02}.wav'),'rb') as audio:
                    self.assertEqual((audio.getnchannels(),audio.getsampwidth(),audio.getframerate()),(2,2,44100))
                    self.assertEqual(audio.getnframes()%588,0)
                    self.assertEqual(audio.readframes(88200),bytes(88200*4))
                    self.assertTrue(any(audio.readframes(44100)))

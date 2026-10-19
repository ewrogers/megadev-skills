#!/usr/bin/env python3
"""Install with telemetry disabled, verify resources, and run the installed generator."""
import os
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def main():
    with tempfile.TemporaryDirectory(prefix='megadev skills install ') as folder:
        cwd=Path(folder);env={**os.environ,'DO_NOT_TRACK':'1','DISABLE_TELEMETRY':'1'}
        subprocess.run(['git','init','-q',str(cwd)],check=True)
        subprocess.run(['npx','--yes','skills@1.7.2','add',str(ROOT),'--skill','megadev-project','megadev-planning','megadev-hardware','megadev-assets','megadev-graphics','megadev-input','megadev-system','megadev-memory','megadev-streaming','megadev-audio','megadev-scenes','megadev-save','megadev-scaling','megadev-debug','megadev-testing','megadev-performance','megadev-release','megadev-best-practices','megadev-ui','megadev-dialogue','megadev-gameplay','megadev-animation','megadev-collision','megadev-levels','megadev-math','megadev-compression','--agent','codex','--yes','--copy'],cwd=cwd,env=env,check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
        for skill_name in ['megadev-project', 'megadev-planning', 'megadev-hardware', 'megadev-assets', 'megadev-graphics', 'megadev-input', 'megadev-system', 'megadev-memory', 'megadev-streaming', 'megadev-audio', 'megadev-scenes', 'megadev-save', 'megadev-scaling','megadev-debug','megadev-testing','megadev-performance','megadev-release','megadev-best-practices','megadev-ui','megadev-dialogue','megadev-gameplay','megadev-animation','megadev-collision','megadev-levels','megadev-math','megadev-compression']:
            installed=cwd/'.agents/skills'/skill_name
            if not installed.exists():raise AssertionError('Installed skill missing: '+skill_name)
            source=ROOT/'skills'/skill_name
            for path in source.rglob('*'):
                if path.is_file() and '__pycache__' not in path.parts:
                    target=installed/path.relative_to(source)
                    assert target.is_file() and target.read_bytes()==path.read_bytes(),str(path)
        installed=cwd/'.agents/skills/megadev-project'
        subprocess.run([sys.executable,str(installed/'scripts/create_project.py'),str(cwd/'standalone game')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (cwd/'standalone game/src/spx.c').is_file()
        planning=cwd/'.agents/skills/megadev-planning'
        report=subprocess.run([sys.executable,str(planning/'scripts/budget_check.py'),str(planning/'assets/budget-example.json'),'--json'],check=True,cwd=cwd,capture_output=True,text=True)
        assert json.loads(report.stdout)['ok']
        hardware=cwd/'.agents/skills/megadev-hardware'
        subprocess.run([sys.executable,str(hardware/'scripts/create_project.py'),str(cwd/'hardware game')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (cwd/'hardware game/src/probe.c').is_file()
        assets=cwd/'.agents/skills/megadev-assets'
        game=cwd/'asset game'
        subprocess.run([sys.executable,str(assets/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        source=game/'res/actor'
        subprocess.run([sys.executable,str(assets/'scripts/prepare_image.py'),str(source.with_suffix('.png')),str(source.with_suffix('.palette.json')),str(game/'res/check.mda'),'--frame','16','24'],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'res/check.mda').read_bytes()==source.with_suffix('.mda').read_bytes()
        subprocess.run([sys.executable,str(assets/'scripts/verify_prepared.py'),'--root',str(game),str(game/'res/check.asset.json')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        graphics=cwd/'.agents/skills/megadev-graphics'
        game=cwd/'graphics game'
        subprocess.run([sys.executable,str(graphics/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/gfx.c').is_file()
        subprocess.run([sys.executable,str(game/'tools/verify_prepared.py'),'--root',str(game),*[str(p) for p in (game/'res').glob('*.asset.json')]],check=True,cwd=cwd,stdout=subprocess.PIPE)
        inputs=cwd/'.agents/skills/megadev-input'
        game=cwd/'input game'
        subprocess.run([sys.executable,str(inputs/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        before=(game/'res/font.bin').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_font.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert (game/'res/font.bin').read_bytes()==before
        assert (game/'src/protocol.c').is_file()
        systems=cwd/'.agents/skills/megadev-system'
        game=cwd/'system game'
        subprocess.run([sys.executable,str(systems/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/main_link.c').is_file() and (game/'src/sub_link.c').is_file()
        before=(game/'res/font.bin').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_font.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert (game/'res/font.bin').read_bytes()==before
        memory=cwd/'.agents/skills/megadev-memory'
        game=cwd/'memory game'
        subprocess.run([sys.executable,str(memory/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/memory_sub.c').is_file() and (game/'layout.json').is_file()
        before=(game/'res/font.bin').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_font.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert (game/'res/font.bin').read_bytes()==before
        missing=subprocess.run([sys.executable,str(game/'tools/report_layout.py'),'--root',str(game)],cwd=cwd,capture_output=True)
        assert missing.returncode!=0, 'Unbuilt project unexpectedly passed linked layout validation'
        streaming=cwd/'.agents/skills/megadev-streaming'
        game=cwd/'streaming game'
        subprocess.run([sys.executable,str(streaming/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/stream_sub.c').is_file() and (game/'res/garden.mda').is_file()
        before=(game/'res/font.bin').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_font.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert (game/'res/font.bin').read_bytes()==before
        audio=cwd/'.agents/skills/megadev-audio'
        game=cwd/'audio game'
        subprocess.run([sys.executable,str(audio/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        subprocess.run([sys.executable,str(game/'tools/make_audio.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert (game/'res/track02.wav').is_file() and (game/'res/bell.pcm').is_file()
        subprocess.run([sys.executable,str(game/'tools/make_voice.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert len((game/'disc/V000.pcm').read_bytes())==16384
        assert len((game/'disc/L000.pcm').read_bytes())==16384
        scenes=cwd/'.agents/skills/megadev-scenes'
        game=cwd/'transition game'
        subprocess.run([sys.executable,str(scenes/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/scene.c').is_file() and (game/'src/audio_sub.c').is_file()
        subprocess.run([sys.executable,str(game/'tools/verify_art.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        saves=cwd/'.agents/skills/megadev-save'
        game=cwd/'save game'
        subprocess.run([sys.executable,str(saves/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/bram.s').is_file() and (game/'src/save.c').is_file()
        subprocess.run([sys.executable,str(game/'tools/make_font.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        scaling=cwd/'.agents/skills/megadev-scaling'
        game=cwd/'scaling game'
        subprocess.run([sys.executable,str(scaling/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        before=(game/'res/stamps.bin').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_art.py')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'res/stamps.bin').read_bytes()==before
        assert (game/'src/scale_sub.c').is_file()
        debug=cwd/'.agents/skills/megadev-debug'
        game=cwd/'debug game'
        subprocess.run([sys.executable,str(debug/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'src/trace.c').is_file() and (game/'tools/lookup_address.py').read_bytes()==(debug/'scripts/lookup_address.py').read_bytes()
        testing=cwd/'.agents/skills/megadev-testing'
        game=cwd/'test game'
        subprocess.run([sys.executable,str(testing/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        result=subprocess.run([sys.executable,str(game/'tools/verify_replay.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(result.stdout)['passed'] is True
        perf=cwd/'.agents/skills/megadev-performance';game=cwd/'perf game'
        subprocess.run([sys.executable,str(perf/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        result=subprocess.run([sys.executable,str(game/'tools/check_work.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(result.stdout)['passed'] is True
        release=cwd/'.agents/skills/megadev-release'
        subprocess.run([sys.executable,str(release/'scripts/release.py'),'--help'],check=True,cwd=cwd,stdout=subprocess.PIPE)
        sys.path.insert(0,str(ROOT/'tests'))
        from test_release import fixture
        from test_project import load
        import zlib
        (cwd/'release-fixture').mkdir()
        cue=fixture(cwd/'release-fixture')
        api=load(release/'scripts/release.py','installed_release')
        api.SIGNATURES={**api.SIGNATURES,'US':(8,zlib.crc32(b'SECURITY'))}
        approved=api.contract(cue,'US','NTSC',cue.parent/'disc',cue.parent/'build')
        provenance={'source_commit':'a'*40,'sdk_commit':'b'*40,'compiler':'distribution fixture','container_reference':'ewrogers/megadev:latest'}
        archive=cwd/'installed-release.zip'
        expected=api.package(cue,approved,provenance,archive)
        assert api.verify(archive)==expected
        practices=cwd/'.agents/skills/megadev-best-practices';game=cwd/'harbor game'
        subprocess.run([sys.executable,str(practices/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        result=subprocess.run([sys.executable,str(game/'tools/check_contracts.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(result.stdout)['passed'] is True
        assert (game/'tools/release.py').read_bytes()==(release/'scripts/release.py').read_bytes()
        ui=cwd/'.agents/skills/megadev-ui';game=cwd/'menu game'
        subprocess.run([sys.executable,str(ui/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        result=subprocess.run([sys.executable,str(game/'tools/check_ui.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(result.stdout)['passed'] is True
        before=(game/'res/fonts.bin').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_assets.py')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'res/fonts.bin').read_bytes()==before
        dialogue=cwd/'.agents/skills/megadev-dialogue';game=cwd/'dialogue game'
        subprocess.run([sys.executable,str(dialogue/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        result=subprocess.run([sys.executable,str(game/'tools/check_dialogue.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(result.stdout)['passed'] is True
        before=(game/'res/voice_welcome.pcm').read_bytes()
        subprocess.run([sys.executable,str(game/'tools/make_audio.py')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (game/'res/voice_welcome.pcm').read_bytes()==before
        gameplay=cwd/'.agents/skills/megadev-gameplay';game=cwd/'adventure game'
        subprocess.run([sys.executable,str(gameplay/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        result=subprocess.run([sys.executable,str(game/'tools/check_gameplay.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(result.stdout)['passed'] is True
        animation=cwd/'.agents/skills/megadev-animation';game=cwd/'animation game'
        subprocess.run([sys.executable,str(animation/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        r=subprocess.run([sys.executable,str(game/'tools/check_animation.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(r.stdout)['passed'] is True
        collision=cwd/'.agents/skills/megadev-collision';game=cwd/'collision game'
        subprocess.run([sys.executable,str(collision/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        r=subprocess.run([sys.executable,str(game/'tools/check_collision.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(r.stdout)['passed'] is True
        levels=cwd/'.agents/skills/megadev-levels';game=cwd/'levels game'
        subprocess.run([sys.executable,str(levels/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        r=subprocess.run([sys.executable,str(game/'tools/check_levels.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(r.stdout)['passed'] is True
        r=subprocess.run([sys.executable,str(levels/'scripts/import_tiled.py'),str(game/'res/world.json'),str(game/'res/world.bin')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(r.stdout)['areas']==2
        maths=cwd/'.agents/skills/megadev-math';game=cwd/'math game'
        subprocess.run([sys.executable,str(maths/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        r=subprocess.run([sys.executable,str(game/'tools/check_math.py')],check=True,capture_output=True,text=True,cwd=cwd)
        assert json.loads(r.stdout)['passed'] is True
        compression=cwd/'.agents/skills/megadev-compression';game=cwd/'compression game'
        subprocess.run([sys.executable,str(compression/'scripts/create_project.py'),str(game)],check=True,cwd=cwd,stdout=subprocess.PIPE)
        subprocess.run([sys.executable,str(game/'tools/make_compressed.py')],check=True,cwd=game,stdout=subprocess.PIPE)
        assert (game/'disc/garden.kos').is_file() and (game/'disc/noise.mda').is_file()
        print('Telemetry-disabled install: all twenty-six skills, standalone generators, planning checker and art/audio tools passed')
if __name__=='__main__':main()

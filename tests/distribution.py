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
        subprocess.run(['npx','--yes','skills@1.7.2','add',str(ROOT),'--skill','megadev-project','megadev-planning','megadev-hardware','megadev-assets','megadev-graphics','--agent','codex','--yes','--copy'],cwd=cwd,env=env,check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
        for skill_name in ['megadev-project', 'megadev-planning', 'megadev-hardware', 'megadev-assets', 'megadev-graphics']:
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
        print('Telemetry-disabled install: all five skills, standalone generators, planning checker and art tools passed')
if __name__=='__main__':main()

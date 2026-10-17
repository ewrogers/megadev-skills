#!/usr/bin/env python3
"""Install with telemetry disabled, verify resources, and run the installed generator."""
import os
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
        subprocess.run(['npx','--yes','skills@1.7.2','add',str(ROOT),'--skill','megadev-project','--agent','codex','--yes','--copy'],cwd=cwd,env=env,check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=180)
        installed=cwd/'.agents/skills/megadev-project'
        if not installed.exists():raise AssertionError('Installed skill missing')
        source=ROOT/'skills/megadev-project'
        for path in source.rglob('*'):
            if path.is_file() and '__pycache__' not in path.parts:
                target=installed/path.relative_to(source)
                assert target.is_file() and target.read_bytes()==path.read_bytes(),str(path)
        subprocess.run([sys.executable,str(installed/'scripts/create_project.py'),str(cwd/'standalone game')],check=True,cwd=cwd,stdout=subprocess.PIPE)
        assert (cwd/'standalone game/src/spx.c').is_file()
        print('Telemetry-disabled install: packaged resources and standalone generator passed')
if __name__=='__main__':main()

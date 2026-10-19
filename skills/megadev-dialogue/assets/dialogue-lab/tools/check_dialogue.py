#!/usr/bin/env python3
"""Run the standalone dialogue contract tests at O0/O3 with a host C compiler."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

def check():
    root=Path(__file__).resolve().parents[1]
    cc=next((p for n in ('cc','clang','gcc') if (p:=shutil.which(n))),None)
    if not cc: raise RuntimeError('A host C compiler is required for executable UI checks')
    with tempfile.TemporaryDirectory() as folder:
        for mode in ('-O0','-O3'):
            exe=Path(folder)/('check'+mode+('.exe' if os.name=='nt' else ''))
            command=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'src/ui.c'),str(root/'src/dialogue.c'),str(root/'src/speech.c'),str(root/'tools/test_dialogue.c'),'-o',str(exe)]
            if os.name!='nt':command+=['-fsanitize=undefined','-fno-sanitize-recover=all']
            r=subprocess.run(command,capture_output=True,text=True)
            if r.returncode:raise RuntimeError(r.stderr)
            r=subprocess.run([str(exe)],capture_output=True,text=True)
            if r.returncode:raise RuntimeError(r.stdout+r.stderr)
    return {'passed':True,'modes':['O0','O3'],'limits':'Host contracts, not VDP or timing emulation'}

if __name__=='__main__':print(json.dumps(check()))

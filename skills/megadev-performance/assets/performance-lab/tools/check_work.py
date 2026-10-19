#!/usr/bin/env python3
"""Compile the portable correctness checks; host execution is not a console benchmark."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

def verify(root,optimization='-O2'):
    root=Path(root);cc=next((p for n in ['cc','clang','gcc'] if (p:=shutil.which(n))),None)
    if not cc:raise ValueError('Host C17 compiler required')
    with tempfile.TemporaryDirectory(prefix='megadev performance ') as folder:
        exe=Path(folder)/('work.exe' if os.name=='nt' else 'work')
        subprocess.run([cc,'-std=c17',optimization,'-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'src/perf.c'),str(root/'tools/work_checks.c'),'-o',str(exe)],check=True,capture_output=True,text=True)
        return subprocess.run([str(exe)],check=True,capture_output=True,text=True).stdout
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);a=p.parse_args()
    try:print(verify(a.root),end='')
    except (ValueError,OSError,subprocess.CalledProcessError) as e:p.exit(2,str(e)+'\n')

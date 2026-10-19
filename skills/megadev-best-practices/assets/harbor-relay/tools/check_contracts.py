#!/usr/bin/env python3
"""Run the small C contract examples with a host C17 compiler."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

def check(root):
    root=Path(root).resolve()
    cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
    if not cc:raise ValueError('A host C17 compiler is required')
    with tempfile.TemporaryDirectory(prefix='megadev contracts ') as folder:
        for opt in ['-O0','-O3']:
            exe=Path(folder)/('contracts'+opt+('.exe' if os.name=='nt' else ''))
            args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'src/contracts.c'),str(root/'tools/contracts_checks.c'),'-o',str(exe)]
            subprocess.run(args,check=True,capture_output=True)
            subprocess.run([str(exe)],check=True,capture_output=True)
    return {'passed':True,'modes':['O0','O3'],'evidence':'host arithmetic/bounds only; not hardware timing'}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);a=p.parse_args()
    print(json.dumps(check(a.root),indent=2))

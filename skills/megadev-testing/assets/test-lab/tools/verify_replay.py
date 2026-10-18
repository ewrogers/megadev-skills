#!/usr/bin/env python3
"""Compile and execute the portable replay regression; JSON on stdout, nonzero on failure."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

def verify(root,compiler=None,optimization='-O2',inject=False):
    root=Path(root).resolve();compiler=compiler or next((p for n in ['cc','clang','gcc'] if (p:=shutil.which(n))),None)
    if not compiler:raise ValueError('A host C17 compiler is required')
    with tempfile.TemporaryDirectory(prefix='megadev replay ') as folder:
        exe=Path(folder)/('replay-check.exe' if os.name=='nt' else 'replay-check')
        cmd=[compiler,'-std=c17',optimization,'-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'src/replay.c'),str(root/'tools/replay_checks.c'),'-o',str(exe)]
        subprocess.run(cmd,check=True,capture_output=True,text=True)
        return subprocess.run([str(exe),*(['--inject-bug'] if inject else [])],capture_output=True,text=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);p.add_argument('--cc');p.add_argument('--inject-bug',action='store_true');a=p.parse_args()
    try:
        result=verify(a.root,a.cc,inject=a.inject_bug)
        print(result.stdout,end='')
        if result.stderr:print(result.stderr,end='',file=__import__('sys').stderr)
        return result.returncode
    except (ValueError,OSError,subprocess.CalledProcessError) as e:
        p.exit(2,str(e)+'\n')
if __name__=='__main__':raise SystemExit(main())

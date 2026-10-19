#!/usr/bin/env python3
"""Run the timeline/consumer contract with debug and optimizing host compilers."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
 if not cc:raise RuntimeError('A host C compiler is required')
 with tempfile.TemporaryDirectory(prefix='animation check ') as folder:
  for opt in ['-O0','-O3']:
   exe=Path(folder)/('animation'+('.exe' if os.name=='nt' else ''))
   args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(ROOT/'src/animation.c'),str(ROOT/'src/training.c'),str(ROOT/'tools/test_animation.c'),'-o',str(exe)]
   if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
   r=subprocess.run(args,capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 return {'passed':True,'configurations':['O0','O3'],'limits':'Model behavior, not rendering or console timing'}
if __name__=='__main__':print(json.dumps(check()))

#!/usr/bin/env python3
"""Check installed fixed-point/motion contracts at O0/O3."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
 if not cc:raise RuntimeError('A host C17 compiler is required')
 with tempfile.TemporaryDirectory(prefix='math check ') as folder:
  for opt in ['-O0','-O3']:
   exe=Path(folder)/('math'+('.exe' if os.name=='nt' else ''))
   args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(ROOT/'src/math_core.c'),str(ROOT/'src/motion.c'),str(ROOT/'tools/test_math.c'),'-o',str(exe)]
   if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
   r=subprocess.run(args,capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 return {'passed':True,'configurations':['O0','O3'],'limits':'Application model; rendering and physical timing need separate checks'}
if __name__=='__main__':print(json.dumps(check()))

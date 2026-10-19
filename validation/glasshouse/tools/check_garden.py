#!/usr/bin/env python3
"""Compile/run the original game and save contracts with strict host checks."""
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
 with tempfile.TemporaryDirectory(prefix='glasshouse host ') as d:
  for opt in ['-O0','-O3']:
   exe=Path(d)/('garden.exe' if os.name=='nt' else 'garden')
   sources=['garden.c','world.c','text_layout.c','ui.c','save.c','contracts.c']
   args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),*[str(ROOT/'src'/n) for n in sources],str(ROOT/'tools/test_garden.c'),'-o',str(exe)]
   if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
   r=subprocess.run(args,capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 return {'passed':True,'configurations':['O0','O3'],'limits':'Game/save models only; BIOS, rendering and CD timing checked separately'}
if __name__=='__main__':print(json.dumps(check()))

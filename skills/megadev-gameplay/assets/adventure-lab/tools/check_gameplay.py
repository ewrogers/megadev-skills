#!/usr/bin/env python3
"""Run the portable gameplay contract at both debug and optimizing settings."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=next((x for n in ['clang','cc','gcc'] if (x:=shutil.which(n))),None)
 if not cc:raise RuntimeError('A host C compiler is needed for the executable check')
 with tempfile.TemporaryDirectory(prefix='gameplay check ') as d:
  for opt in ['-O0','-O3']:
   exe=Path(d)/('gameplay'+('.exe' if os.name=='nt' else ''))
   args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(ROOT/'src/gameplay.c'),str(ROOT/'tools/test_gameplay.c'),'-o',str(exe)]
   if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
   subprocess.run(args,check=True,capture_output=True,text=True)
   subprocess.run([str(exe)],check=True,capture_output=True,text=True)
 return {'passed':True,'configurations':['O0','O3'],'limits':'Portable model; not console timing or VDP emulation'}
if __name__=='__main__':print(json.dumps(check()))

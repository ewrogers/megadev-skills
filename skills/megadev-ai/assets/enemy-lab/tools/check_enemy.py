#!/usr/bin/env python3
"""Execute the portable AI contracts at O0/O3 with undefined-behavior checks."""
import json,os,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
 if not cc:raise RuntimeError('A host C17 compiler is required')
 with tempfile.TemporaryDirectory(prefix='enemy check ') as folder:
  for opt in ['-O0','-O3']:
   exe=Path(folder)/('enemy.exe' if os.name=='nt' else 'enemy')
   args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(ROOT/'src/enemy.c'),str(ROOT/'tools/test_enemy.c'),'-o',str(exe)]
   if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
   r=subprocess.run(args,capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True)
   if r.returncode:raise RuntimeError(f'AI contract failed (exit {r.returncode}): {r.stderr}')
 return {'passed':True,'configurations':['O0','O3'],'limits':'Portable behavior and work bounds, not console cycle or VDP timing'}
if __name__=='__main__':print(json.dumps(check()))

#!/usr/bin/env python3
"""Execute roster, camera, respawn and equal-time regional checks at O0/O3."""
import json,os,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
 if not cc:raise RuntimeError('C17 compiler required')
 with tempfile.TemporaryDirectory(prefix='coop check ') as folder:
  for opt in ['-O0','-O3']:
   exe=Path(folder)/('coop.exe' if os.name=='nt' else 'coop')
   cmd=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(ROOT/'src'),str(ROOT/'src/coop.c'),str(ROOT/'src/input.c'),str(ROOT/'tools/test_coop.c'),'-o',str(exe)]
   # The Windows runner's host compiler need not have a UBSan runtime.
   if os.name!='nt':cmd+=['-fsanitize=undefined','-fno-sanitize-recover=all']
   r=subprocess.run(cmd,capture_output=True,text=True)
   if r.returncode:raise RuntimeError(r.stderr)
   r=subprocess.run([str(exe)],capture_output=True,text=True)
   if r.returncode:raise RuntimeError(f'Co-op contract failed (exit {r.returncode}): {r.stderr}')
 return {'passed':True,'models':['roster','disconnect','camera','respawn','pause','PAL/NTSC'],'undefined_behavior_sanitizer':os.name!='nt'}
if __name__=='__main__':print(json.dumps(check()))

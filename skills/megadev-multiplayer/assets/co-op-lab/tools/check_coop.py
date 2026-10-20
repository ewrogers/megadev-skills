#!/usr/bin/env python3
"""Execute roster, camera, respawn and equal-time regional checks."""
import json,shutil,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=shutil.which('cc') or shutil.which('clang') or shutil.which('gcc')
 if not cc:raise RuntimeError('C17 compiler required')
 with tempfile.TemporaryDirectory() as t:
  for opt in ['-O0','-O3']:
   exe=Path(t)/'coop'
   cmd=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=undefined','-fno-sanitize-recover=all','-I'+str(ROOT/'src'),str(ROOT/'src/coop.c'),str(ROOT/'src/input.c'),str(ROOT/'tools/test_coop.c'),'-o',str(exe)]
   subprocess.run(cmd,check=True,capture_output=True);subprocess.run([str(exe)],check=True)
 return {'passed':True,'models':['roster','disconnect','camera','respawn','pause','PAL/NTSC']}
if __name__=='__main__':print(json.dumps(check()))

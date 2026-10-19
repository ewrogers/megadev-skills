#!/usr/bin/env python3
"""Run installed portable model contracts at O0/O3; requires a host C17 compiler."""
import os,shutil,subprocess,tempfile
from pathlib import Path
def check(root):
 root=Path(root);cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
 if not cc:raise RuntimeError('Install a host C17 compiler for portable checks')
 results=[]
 with tempfile.TemporaryDirectory() as folder:
  for opt in ['-O0','-O3']:
   exe=Path(folder)/('test.exe' if os.name=='nt' else 'test')
   subprocess.run([cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(root/'src'),str(root/'src/cinema.c'),str(root/'tools/test_cinema.c'),'-o',str(exe)],check=True)
   results.append(subprocess.check_output([str(exe)],text=True))
 assert results[0]==results[1];return results[0].strip()
if __name__=='__main__':print(check(Path(__file__).resolve().parents[1]))

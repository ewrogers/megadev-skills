#!/usr/bin/env python3
"""Run the standalone portable language/layout/publication contracts."""
from pathlib import Path
import argparse,json,os,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def check():
 cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
 if not cc:raise RuntimeError('A C17 compiler is required for host contracts')
 results=[]
 with tempfile.TemporaryDirectory() as d:
  for level in ['-O0','-O3']:
   binary=Path(d)/('check.exe' if os.name=='nt' else 'check')
   command=[cc,'-std=c17',level,'-I',str(ROOT/'src'),str(ROOT/'tools/test_language.c'),*[str(ROOT/'src'/n) for n in ['loc.c','catalog.c','ui.c','paint.c']],'-o',str(binary)]
   subprocess.run(command,check=True,capture_output=True,text=True)
   r=subprocess.run([str(binary)],check=True,capture_output=True,text=True);results.append(json.loads(r.stdout))
 if results[0]!=results[1]:raise AssertionError('O0/O3 language behavior differs')
 return results[0]
if __name__=='__main__':print(json.dumps(check()))

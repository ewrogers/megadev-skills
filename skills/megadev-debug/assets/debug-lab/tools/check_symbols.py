#!/usr/bin/env python3
"""Qualification check against each exact linked Lab ELF (run in toolchain)."""
import hashlib
from pathlib import Path
import subprocess
import sys
from lookup_address import resolve
root=Path(sys.argv[1]);debug=len(sys.argv)>2 and sys.argv[2]=='debug'
for module,file,function,cpu in [('main','ipx.mmd.elf','sy_main_tick','main'),('sub','spx.smd.elf','sy_sub_tick','sub'),('room','room.mmd.elf','main','main')]:
    elf=root/'build'/file;sha=hashlib.sha256(elf.read_bytes()).hexdigest()
    nm=subprocess.run(['m68k-elf-nm','-n',str(elf)],check=True,capture_output=True,text=True).stdout
    pc=int(next(line.split()[0] for line in nm.splitlines() if line.split()[-1]==function),16)
    report=resolve(elf,cpu,module,pc,sha)
    if debug:assert report['source_available'],report
    try:resolve(elf,'sub' if cpu=='main' else 'main',module,pc,sha)
    except ValueError:pass
    else:raise AssertionError('Wrong CPU accepted')
    print(module,hex(pc),report['location'].replace('\n',' / '))

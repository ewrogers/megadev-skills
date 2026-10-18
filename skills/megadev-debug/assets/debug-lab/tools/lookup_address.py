#!/usr/bin/env python3
"""Resolve a PC using an explicitly selected CPU/module and exact linked ELF."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

def resolve(elf,cpu,module,pc,expected_sha256,prefix='m68k-elf-'):
    elf=Path(elf)
    actual=hashlib.sha256(elf.read_bytes()).hexdigest()
    if actual!=expected_sha256:raise ValueError('ELF SHA-256 differs from captured build identity')
    roles={'main':('ipx.mmd.elf',0xFF0000,0xFF8000),'sub':('spx.smd.elf',0x10000,0x80000),'room':('room.mmd.elf',0x200100,0x240000)}
    if module not in roles:raise ValueError('Unknown Lab module; use this project-specific map only for this Lab')
    filename,low,high=roles[module]
    if cpu!=('sub' if module=='sub' else 'main') or elf.name!=filename:raise ValueError('CPU/module/ELF identity mismatch')
    if pc&1 or not low<=pc<high:raise ValueError('PC outside selected module code window or not word aligned')
    # Linker bounds exclude RAM data and unused space; no nearest-symbol guess.
    nm=subprocess.run([prefix+'nm','-n',str(elf)],check=True,capture_output=True,text=True).stdout
    table={m[2]:int(m[1],16) for line in nm.splitlines() if (m:=re.fullmatch(r'([0-9a-fA-F]+)\s+\S\s+(\S+)',line))}
    start=table.get('_TEXT_ORIGIN');size=table.get('_TEXT_LENGTH')
    if start is None or size is None or not start<=pc<start+size:raise ValueError('PC is not in this ELF text section')
    location=subprocess.run([prefix+'addr2line','-f','-C','-e',str(elf),hex(pc)],check=True,capture_output=True,text=True).stdout.strip()
    return {'cpu':cpu,'module':module,'pc':hex(pc),'elf_sha256':actual,'location':location,'source_available':not location.endswith(':0') and '??' not in location}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('elf',type=Path)
    p.add_argument('--cpu',required=True,choices=['main','sub']);p.add_argument('--module',required=True,choices=['main','sub','room'])
    p.add_argument('--pc',required=True,type=lambda x:int(x,0));p.add_argument('--sha256',required=True);p.add_argument('--prefix',default='m68k-elf-')
    a=p.parse_args()
    try:print(json.dumps(resolve(a.elf,a.cpu,a.module,a.pc,a.sha256,a.prefix),indent=2))
    except (ValueError,OSError,subprocess.CalledProcessError) as e:p.exit(2,str(e)+'\n')
if __name__=='__main__':main()

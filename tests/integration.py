#!/usr/bin/env python3
"""Build fresh standalone Project Labs and validate their target payloads/startup."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from test_project import ROOT,SKILL,create
from startup import symbols,check,check_sub_callback
sys.path.insert(0,str(SKILL/'scripts'))
from inspect_disc import inspect
from disc import digest

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image',default='ewrogers/megadev:latest')
    parser.add_argument('--platform',required=True,choices=['linux/arm64','linux/amd64'])
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=['docker','run','--rm','--platform',args.platform]
    if hasattr(os,'getuid'):base+=['--user',f'{os.getuid()}:{os.getgid()}']
    base+=['--mount',f'type=bind,source={output},target=/workspace',args.image]
    def run(name,command,success=True):
        with (output/(name+'.log')).open('w') as f:
            r=subprocess.run(base+command,stdout=f,stderr=subprocess.STDOUT,timeout=240)
        text=(output/(name+'.log')).read_text()
        if success and r.returncode:raise RuntimeError(name+': '+text[-2500:])
        if not success and not r.returncode:raise AssertionError(name+' unexpectedly succeeded')
        return text
    doctor=run('doctor',['doctor']);assert '-fno-strict-aliasing' in doctor
    cases={}
    for key,region,video,flags in [('us-release','US','NTSC',[]),('jp-release','JP','NTSC',[]),
                                  ('eu-release','EU','PAL',[]),('us-debug','US','NTSC',['OPT_FLAGS=-Og -g -fconserve-stack -fomit-frame-pointer','DEBUG=1'])]:
        project=create(output/(key+' game'))
        relative=str(project.relative_to(output));run(key,['build','-C',relative,'REGION='+region,'VIDEO='+video,*flags])
        # The upstream boot linker uses RWX LOAD segments on bare-metal RAM.
        log=(output/(key+'.log')).read_text()
        unexpected=[line for line in log.splitlines() if 'warning:' in line and 'has a LOAD segment with RWX permissions' not in line]
        assert not unexpected,unexpected
        image=project/'project_lab.iso'
        got=inspect(image,region,['IPX.MMD','SPX.SMD','ROOM.MMD'])
        for wrong_region, expected in [('JP' if region!='JP' else 'US', []), (region, ['MISSING.MMD'])]:
            try:inspect(image,wrong_region,expected)
            except ValueError:pass
            else:raise AssertionError('Inspector accepted a wrong region or missing module')
        payloads={file.name.upper():digest(file.read_bytes()) for file in (project/'disc').iterdir() if file.is_file()}
        assert got['files']==payloads,'disc contents differ from compiled modules'
        run(key+'-cue',['cue',relative+'/project_lab.iso'])
        assert 'MODE1/2048' in (project/'project_lab.cue').read_text()
        startup={}
        for file,kind in [('ipx.mmd','main'),('spx.smd','sub'),('room.mmd','room')]:
            table=symbols((project/'build'/(file+'.elf.sym')).read_text())
            startup[kind]=check((project/'disc'/file).read_bytes(),table,kind)
        sp=symbols((project/'build/sp.bin.elf.sym').read_text())
        assert check_sub_callback((project/'build/sp.bin').read_bytes(),sp)
        cases[key]={'disc':got,'startup':startup,'sub_callback':True,
                    'boot_payloads':{file.name:digest(file.read_bytes()) for file in (project/'build').glob('*.bin')}}
        shutil.copyfile(image,output/(key+'.iso'));(output/(key+'.cue')).write_text((project/'project_lab.cue').read_text().replace('project_lab.iso',key+'.iso'))
        # Keep one self-contained review directory; copied CUE still refers to
        # project_lab.iso within each original project folder.
        print('Passed '+key,flush=True)
    bad=create(output/'oversized allocation')
    layout=bad/'src/ipx_layout.s';layout.write_text(layout.read_text().replace('MODULE_RAM_LENGTH 0x800','MODULE_RAM_LENGTH 0x4'))
    log=run('reject-overlap',['build','-C',str(bad.relative_to(output))],success=False)
    assert 'will not fit' in log or 'overflowed' in log,'expected linker capacity rejection'
    report={'passed':True,'platform':args.platform,'doctor':doctor,'cases':cases,
            'limits':'M68000 startup/callback execution only; BIOS, full CD I/O, IRQ timing and peripherals need emulator/hardware checks'}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Passed '+args.platform+'; report: '+str(output/'report.json'))
if __name__=='__main__':main()

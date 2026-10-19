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
from test_project import ROOT,SKILL,create,load
from startup import symbols,check,check_sub_callback
sys.path.insert(0,str(SKILL/'scripts'))
from inspect_disc import inspect
from disc import digest

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--skill',default='megadev-project',choices=['megadev-project','megadev-hardware','megadev-assets','megadev-graphics','megadev-input','megadev-system','megadev-memory','megadev-streaming','megadev-audio','megadev-scenes','megadev-save','megadev-scaling','megadev-debug','megadev-testing','megadev-performance','megadev-best-practices'])
    parser.add_argument('--image',default='ewrogers/megadev:latest')
    parser.add_argument('--platform',required=True,choices=['linux/arm64','linux/amd64'])
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    creator=load(ROOT/'skills'/args.skill/'scripts/create_project.py','selected_creator').create
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    base=['docker','run','--rm','--cpus=2','--platform',args.platform]
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
    def fixture_times(project):
        # Docker Desktop's guest clock can lag the host. These are disposable
        # fresh-build inputs; stable old mtimes avoid misleading make skew warnings.
        for path in [*project.rglob('*'),project]:
            os.utime(path,(946684800,946684800),follow_symlinks=False)
    matrix=[('us-release','US','NTSC',[]),('jp-release','JP','NTSC',[]),
                                  ('eu-release','EU','PAL',[]),('us-debug','US','NTSC',['OPT_FLAGS=-Og -g -fconserve-stack -fomit-frame-pointer','DEBUG=1'])]
    if args.skill in ('megadev-hardware','megadev-assets','megadev-graphics','megadev-input','megadev-system','megadev-memory','megadev-streaming','megadev-audio','megadev-scenes','megadev-save','megadev-scaling','megadev-debug','megadev-testing','megadev-performance','megadev-best-practices'):matrix.append(('us-tour','US','NTSC',['TOUR=1']))
    if args.skill=='megadev-input':matrix.append(('us-ea','US','NTSC',['EA_MODE=1']))
    for key,region,video,flags in matrix:
        project=creator(output/(key+' game'),project_id='project_lab')
        relative=str(project.relative_to(output))
        if args.skill=='megadev-assets' and key=='us-release':
            sys.path.insert(0,str(ROOT/'skills/megadev-assets/scripts'))
            from prepare_image import prepare
            for name in ['garden','cavern','actor','panel']:
                source=project/'res'
                prepared=project/'reprepared'/(name+'.mda')
                art=prepare(source/(name+'.png'),source/(name+'.palette.json'),prepared,(16,24) if name=='actor' else None,backdrop=name in ('garden','cavern'))
                assert prepared.read_bytes()==(source/(name+'.mda')).read_bytes(),'prepared bytes differ'
                assert not art['snapped_source_colors'],'unexpected color changes'
        fixture_times(project)
        run(key,['build','-C',relative,'REGION='+region,'VIDEO='+video,*flags])
        # The upstream boot linker uses RWX LOAD segments on bare-metal RAM.
        log=(output/(key+'.log')).read_text()
        unexpected=[line for line in log.splitlines() if 'warning:' in line and 'has a LOAD segment with RWX permissions' not in line]
        assert not unexpected,unexpected
        image=project/'project_lab.iso'
        expected_files=['IPX.MMD','SPX.SMD','ROOM.MMD']
        if args.skill in ('megadev-assets','megadev-streaming','megadev-audio','megadev-scenes','megadev-testing','megadev-best-practices'):expected_files+=['GARDEN.MDA','CAVERN.MDA']
        if args.skill=='megadev-graphics':expected_files+=['SKY.MDA','QUAY.MDA']
        got=inspect(image,region,expected_files)
        for wrong_region, expected in [('JP' if region!='JP' else 'US', []), (region, ['MISSING.MMD'])]:
            try:inspect(image,wrong_region,expected)
            except ValueError:pass
            else:raise AssertionError('Inspector accepted a wrong region or missing module')
        payloads={file.name.upper():digest(file.read_bytes()) for file in (project/'disc').iterdir() if file.is_file()}
        assert got['files']==payloads,'disc contents differ from compiled modules'
        tracks={}
        if args.skill in ('megadev-audio','megadev-scenes','megadev-testing','megadev-best-practices'):
            cue_tool=load(project/'tools/make_cue.py','audio_cue')
            cue_tool.write_cue(image)
            for track in (2,3):
                path=project/'res'/f'track{track:02}.wav'
                tracks[str(track)]=digest(path.read_bytes())
                destination=output/'res'/path.name;destination.parent.mkdir(exist_ok=True)
                if destination.exists():assert destination.read_bytes()==path.read_bytes(),'Region changed audio payload'
                else:shutil.copyfile(path,destination)
        else:run(key+'-cue',['cue',relative+'/project_lab.iso'])
        assert 'MODE1/2048' in (project/'project_lab.cue').read_text()
        startup={}
        for file,kind in [('ipx.mmd','main'),('spx.smd','sub'),('room.mmd','room')]:
            table=symbols((project/'build'/(file+'.elf.sym')).read_text())
            extra=None
            if args.skill=='megadev-hardware' and kind in ('main','sub'):
                from hardware_cpu import check_probe
                extra=lambda u,sym:check_probe(u,sym,kind)
            if args.skill=='megadev-assets' and kind=='main':
                from assets_cpu import check_assets
                extra=lambda u,sym:check_assets(u,sym,project/'res')
            if args.skill=='megadev-graphics' and kind=='main':
                from graphics_cpu import check_graphics
                extra=check_graphics
            if args.skill=='megadev-input' and kind=='main':
                from input_cpu import check_input
                extra=lambda u,sym:check_input(u,sym,key=='us-ea')
            startup[kind]=check((project/'disc'/file).read_bytes(),table,kind,extra)
        if args.skill=='megadev-system':
            from system_cpu import check_system
            assert check_system(project,50 if video=='PAL' else 60)
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
        if args.skill=='megadev-debug':
            run(key+'-symbols',['python3',relative+'/tools/check_symbols.py',relative,'debug' if key=='us-debug' else 'release'])
            from debug_cpu import check_debug
            assert check_debug(project,50 if video=='PAL' else 60)
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
            startup['main']['debug_execution']=True
        if args.skill=='megadev-memory':
            from memory_cpu import check_memory
            assert check_memory(project,50 if video=='PAL' else 60)
            layout=load(project/'tools/report_layout.py','memory_layout').report(project)
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
            startup['main']['layout']=layout
        if args.skill=='megadev-streaming':
            from streaming_cpu import check_streaming
            assert check_streaming(project,50 if video=='PAL' else 60)
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
        if args.skill in ('megadev-audio','megadev-scenes','megadev-testing','megadev-best-practices'):
            from audio_cpu import check_audio
            from streaming_cpu import check_streaming
            assert check_audio(project,50 if video=='PAL' else 60)
            if args.skill!='megadev-testing':
                assert check_streaming(project,50 if video=='PAL' else 60,entry_controls=False)
            if args.skill in ('megadev-scenes','megadev-best-practices'):
                from scenes_cpu import check_scenes
                assert check_scenes(project,50 if video=='PAL' else 60)
                startup['main']['scene_execution']=True
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
        if args.skill=='megadev-testing':
            from testing_cpu import check_testing
            assert check_testing(project,50 if video=='PAL' else 60)
            startup['main']['replay_execution']=True
            startup['main']['integration_execution']=True
        if args.skill=='megadev-save':
            from save_cpu import check_save
            assert check_save(project,50 if video=='PAL' else 60)
            startup['main']['cart_execution']=True
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
        if args.skill=='megadev-scaling':
            from scaling_cpu import check_scaling
            assert check_scaling(project,50 if video=='PAL' else 60)
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
            startup['sub']['asic_execution']=True
        if args.skill=='megadev-performance':
            from performance_cpu import check_performance
            assert check_performance(project,50 if video=='PAL' else 60)
            startup['main']['probe_execution']=True
            startup['sub']['probe_execution']=True
            startup['sub']['asic_execution']=True
            startup['main']['measurement_execution']=True
        if args.skill=='megadev-best-practices':
            from practices_cpu import check_practices
            assert check_practices(project,50 if video=='PAL' else 60)
            startup['main']['practice_execution']=True
            startup['sub']['checkpoint_execution']=True
        sp=symbols((project/'build/sp.bin.elf.sym').read_text())
        assert check_sub_callback((project/'build/sp.bin').read_bytes(),sp)
        cases[key]={'disc':got,'startup':startup,'sub_callback':True,
                    'boot_payloads':{file.name:digest(file.read_bytes()) for file in (project/'build').glob('*.bin')}}
        if tracks:cases[key]['audio_tracks']=tracks
        if args.skill in ('megadev-audio','megadev-best-practices'):
            from release_integration import check_release
            cases[key]['release']=check_release(project,region,video,doctor,args.image)
        shutil.copyfile(image,output/(key+'.iso'));(output/(key+'.cue')).write_text((project/'project_lab.cue').read_text().replace('project_lab.iso',key+'.iso'))
        # Keep one self-contained review directory; copied CUE still refers to
        # project_lab.iso within each original project folder.
        print('Passed '+key,flush=True)
    bad=creator(output/'oversized allocation',project_id='project_lab')
    import re
    layout=bad/'src/ipx_layout.s';layout.write_text(re.sub(r'MODULE_RAM_LENGTH 0x[0-9A-Fa-f]+','MODULE_RAM_LENGTH 0x4',layout.read_text()))
    fixture_times(bad)
    log=run('reject-overlap',['build','-C',str(bad.relative_to(output))],success=False)
    assert 'will not fit' in log or 'overflowed' in log,'expected linker capacity rejection'
    report={'passed':True,'skill':args.skill,'platform':args.platform,'doctor':doctor,'cases':cases,
            'limits':'Compiled M68000 startup and selected model/callback execution; MMIO capture is not a VDP. BIOS, full CD I/O, IRQ timing and peripherals need emulator/hardware checks'}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Passed '+args.platform+'; report: '+str(output/'report.json'))
if __name__=='__main__':main()

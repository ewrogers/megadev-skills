"""Qualify release tooling against the unmodified compiled mixed-mode Audio Lab."""
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile
from test_project import ROOT,load


def check_release(project,region,video,doctor,image):
    api=load(ROOT/'skills/megadev-release/scripts/release.py','release_integration_api')
    cue=project/'project_lab.cue'
    approved=api.contract(cue,region,video,project/'disc',project/'build')
    sdk=re.search(r'^Megadev: ([0-9a-f]{40})$',doctor,re.M)
    compiler=re.search(r'^GCC: (.+)$',doctor,re.M)
    assert sdk and compiler,'Toolchain identity missing'
    provenance={'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                'sdk_commit':sdk[1],'compiler':'m68k-elf-gcc '+compiler[1],'container_reference':image}
    # Explicit notices only. Unrelated project files/build outputs never enter the ZIP.
    shutil.copyfile(ROOT/'LICENSE',project/'LICENSE')
    include=['README.md','LICENSE','LICENSE.megadev']
    first=project/'release.zip';second=project/'release-repeat.zip'
    manifest=api.package(cue,approved,provenance,first,include)
    assert api.verify(first)==manifest
    api.package(cue,approved,provenance,second,include)
    assert first.read_bytes()==second.read_bytes(),'Same inputs produced different archives'
    (project/'release-contract.json').write_bytes(api.encoded(approved))
    (project/'release-provenance.json').write_bytes(api.encoded(provenance))
    # Real command-line path, from a working directory outside the project.
    subprocess.run([sys.executable,str(ROOT/'skills/megadev-release/scripts/release.py'),'verify',str(first.resolve())],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
    failures=[]
    def rejects(name,fn):
        try:fn()
        except (ValueError,OSError):failures.append(name)
        else:raise AssertionError(name+' was accepted')
    audio=project/'res/track02.wav';original=audio.read_bytes()
    audio.unlink();rejects('missing_track',lambda:api.validate(cue,approved));audio.write_bytes(original)
    bad=bytearray(original);bad[-5]^=1;audio.write_bytes(bad)
    rejects('changed_audio',lambda:api.validate(cue,approved));audio.write_bytes(original)
    text=cue.read_text();cue.write_text(text.replace('00:02:00','00:01:00'))
    rejects('changed_pregap',lambda:api.validate(cue,approved));cue.write_text(text)
    cue.write_text(text.replace('track02.wav','missing.wav'))
    rejects('wrong_track_path',lambda:api.validate(cue,approved));cue.write_text(text)
    iso=project/'project_lab.iso';data=iso.read_bytes();bad=bytearray(data);bad[0x200]^=1;iso.write_bytes(bad)
    rejects('security_damage',lambda:api.validate(cue,approved));iso.write_bytes(data)
    # Hashes alone are not authentication, but a rewritten file hash must not
    # bypass the independently approved disc contract inside this package.
    damaged=project/'release-damaged.zip'
    with zipfile.ZipFile(first) as source,zipfile.ZipFile(damaged,'w') as out:
        for name in source.namelist():
            value=source.read(name)
            if name=='res/track02.wav':value=value[:-4]+b'BAD!'
            out.writestr(name,value)
    rejects('damaged_package',lambda:api.verify(damaged))
    assert api.validate(cue,approved)==approved['disc'],'Failure injection altered release inputs'
    # Do not compare ISO timestamps or call injected/structural checks hardware evidence.
    return {'profile':'megadev-multifile-v1','round_trip':True,'reproducible_package':True,
            'region':region,'video':video,'negative_cases':failures,
            'files':approved['disc']['iso']['files'],'boot':approved['compiled_boot'],
            'tracks':approved['disc']['tracks']}

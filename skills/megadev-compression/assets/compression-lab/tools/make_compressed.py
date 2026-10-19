#!/usr/bin/env python3
"""Original deterministic Kosinski encoder for the audited Megadev decoder."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import zlib
ROOT=Path(__file__).resolve().parents[1]
_spec=importlib.util.spec_from_file_location('compression_art_validation',Path(__file__).with_name('verify_art.py'))
_art=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_art)
validate=_art.validate

class Writer:
    def __init__(self):self.data=bytearray(2);self.desc=0;self.used=0
    def bit(self,value):
        self.data[self.desc+self.used//8]|=value<<(self.used%8);self.used+=1
        if self.used==16:self.desc=len(self.data);self.data+=bytes(2);self.used=0
    def payload(self,*values):self.data.extend(values)

def encode(data):
    if not data or len(data)>32768:raise ValueError('Expected 1-32768 source bytes')
    out=Writer();seen={};pos=0
    while pos<len(data):
        best=0;distance=0;key=data[pos:pos+3]
        for earlier in reversed(seen.get(key,[])[-64:]):
            d=pos-earlier
            if d>8192:break
            count=0
            while count<min(256,len(data)-pos) and data[pos+count]==data[pos+count-d]:count+=1
            if count>best:best=count;distance=d
            if best==256:break
        length=best if best>=3 else 1
        if length==1:out.bit(1);out.payload(data[pos])
        else:
            out.bit(0);out.bit(1);offset=(-distance)&8191
            hi=(offset>>5)&248
            if length<=9:out.payload(offset&255,hi|(length-2))
            else:out.payload(offset&255,hi,length-1)
        for i in range(length):
            key=data[pos+i:pos+i+3];candidates=seen.setdefault(key,[]);candidates.append(pos+i)
            if len(candidates)>64:del candidates[:-64]
        pos+=length
    out.bit(0);out.bit(1);out.payload(0,0,0)
    return bytes(out.data)

def noise(source):
    out=bytearray(source);size=int.from_bytes(out[16:20],'big');rng=0x19370102
    for i in range(64,64+size):
        rng^=(rng<<13)&0xffffffff;rng^=rng>>17;rng^=(rng<<5)&0xffffffff;out[i]=rng&255
    out[24:28]=struct.pack('>I',zlib.crc32(out[32:]));return bytes(out)

def generate(root=ROOT):
    root=Path(root);disc=root/'disc';build=root/'build';disc.mkdir(exist_ok=True);build.mkdir(exist_ok=True)
    raw=[(root/'res'/f'{n}.mda').read_bytes() for n in ['garden','cavern']]
    raw.append(noise(raw[0]));(root/'res/noise.mda').write_bytes(raw[2])
    sizes=[];decoded=[];names=[];manifest=[]
    for i,name in enumerate(['GARDEN','CAVERN','NOISE']):
        validate(root/'res'/f'{name.lower()}.mda');packed=encode(raw[i])
        for ext,data in [('MDA',raw[i]),('KOS',packed)]:
            rounded=(len(data)+2047)&~2047
            if rounded>32768:raise ValueError('Rounded input exceeds 32 KiB staging')
            (disc/f'{name.lower()}.{ext.lower()}').write_bytes(data)
            names.append(f'"{name}.{ext};1"');sizes.append(len(data));decoded.append(len(raw[i]))
            manifest.append({'file':f'{name}.{ext}','bytes':len(data),'sectors':rounded//2048,
                             'output_bytes':len(raw[i]),'active_buffer_bytes':rounded+len(raw[i]),
                             'reserved_word_ram_bytes':65536,'sha256':hashlib.sha256(data).hexdigest()})
    def array(name,values):return f'static unsigned short const {name}[]={{'+','.join(map(str,values))+'};\n'
    header='/* Generated known asset sizes, not data supplied by a disc header. */\n'
    header+='static char const * const cp_files[]={'+','.join(names)+'};\n'
    header+=array('cp_sizes',sizes)+array('cp_outputs',decoded)
    target=build/'compression_assets.h'
    if not target.exists() or target.read_text()!=header:target.write_text(header)
    (build/'compression-assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest
if __name__=='__main__':generate()

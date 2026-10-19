#!/usr/bin/env python3
"""Prepare bounded RF5C164 chunks; authoring uses only the Python standard library."""
import hashlib
import json
import math
from pathlib import Path
import struct
import wave

ROOT=Path(__file__).resolve().parents[1]
RATE=8000
HALF=16384
PAYLOAD=HALF-1

def encode(value):
    value=max(-126,min(126,value))
    return 0x80|value if value>=0 else -value

def read_voice(path):
    with wave.open(str(path),'rb') as audio:
        if audio.getnchannels()!=1 or audio.getsampwidth()!=2 or audio.getcomptype()!='NONE':
            raise ValueError('Voice must be mono, signed PCM16 WAV')
        rate=audio.getframerate();count=audio.getnframes()
        if rate!=RATE or not RATE*9<=count<=RATE*60:
            raise ValueError('Voice must be 9-60 seconds at 8000 Hz; resample with an anti-alias filter before importing')
        samples=struct.unpack('<'+'h'*count,audio.readframes(count))
    out=bytearray()
    for value in samples:
        out.append(encode(round(value*126/32768)))
    return bytes(out)

def long_sample():
    # Original long, gently evolving chimes: no external musical recording.
    out=bytearray()
    for i in range(RATE*14):
        t=i/RATE;beat=t%1.75;note=[220,277.18,329.63,415.30][int(t/1.75)%4]
        envelope=min(1,beat/.02)*math.exp(-beat*1.6)*min(1,(14-t)/.2)
        value=(math.sin(2*math.pi*note*t)+.25*math.sin(2*math.pi*note*2.01*t))*envelope*.6
        out.append(encode(round(value*126)))
    return bytes(out)

def split(data):
    if not data or 0xff in data:raise ValueError('PCM payload must not contain the loop marker')
    return [data[i:i+PAYLOAD].ljust(PAYLOAD,b'\x80')+b'\xff' for i in range(0,len(data),PAYLOAD)]

def generate(root=ROOT):
    root=Path(root);disc=root/'disc';build=root/'build'
    disc.mkdir(exist_ok=True);build.mkdir(exist_ok=True)
    manifest={'rate':RATE,'pcm_buffer_bytes':HALF*2,'sub_staging_bytes':HALF,'clips':{}}
    header=['/* Generated sector-sized files; data never contains FF. */',
            f'#define VOICE_STEP {(RATE*2048*384+6250000)//12500000}u']
    expected=set()
    for name,data in [('V',read_voice(root/'res/stream_voice.wav')),('L',long_sample())]:
        chunks=split(data)
        if len(chunks)>99:raise ValueError('Clip exceeds demo directory budget')
        checks=[];files={}
        for i,chunk in enumerate(chunks):
            filename=f'{name}{i:03}.pcm';path=disc/filename;expected.add(filename)
            if not path.exists() or path.read_bytes()!=chunk:path.write_bytes(chunk)
            checks.append(sum(chunk)&65535);files[filename]=hashlib.sha256(chunk).hexdigest()
        header.extend([f'#define VOICE_{name}_BLOCKS {len(chunks)}u',
                       f'static unsigned short const voice_{name.lower()}_sums[]={{'+','.join(str(n) for n in checks)+'};'])
        manifest['clips'][name]={'bytes':len(data),'padded_bytes':len(chunks)*HALF,'files':files}
    # This script owns only these generated filenames in a disposable project.
    for pattern in ('V[0-9][0-9][0-9].pcm','L[0-9][0-9][0-9].pcm'):
        for path in disc.glob(pattern):
            if path.name not in expected:path.unlink()
    target=build/'voice_assets.h';text='\n'.join(header)+'\n'
    if not target.exists() or target.read_text()!=text:target.write_text(text)
    (build/'voice-assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest

if __name__=='__main__':generate()

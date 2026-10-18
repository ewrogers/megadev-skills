#!/usr/bin/env python3
"""Original reproducible Lab music and PCM cues; no external samples or libraries."""
from array import array
import hashlib
import json
import math
from pathlib import Path
import struct
import wave
ROOT=Path(__file__).resolve().parents[1]

def pcm_encode(value):
    n=max(-126,min(126,round(value*126)))
    return (0x80|n) if n>=0 else -n

def pcm_sample(name):
    lengths={'bed':2048,'bell':3072,'pluck':2048,'drum':1536};rate=8000 if name=='bed' else 11025
    out=bytearray()
    for i in range(lengths[name]):
        t=i/rate;u=i/lengths[name]
        if name=='bed':v=.30*math.sin(2*math.pi*i/64)+.12*math.sin(2*math.pi*i/32)
        elif name=='bell':v=(math.sin(2*math.pi*880*t)+.35*math.sin(2*math.pi*2200*t))*math.exp(-18*t)*min(1,i/24)*.62
        elif name=='pluck':v=(math.sin(2*math.pi*440*t)+.23*math.sin(2*math.pi*1320*t))*math.exp(-22*t)*min(1,i/24)*.72
        else:v=math.sin(2*math.pi*(100*t+3*(1-math.exp(-22*t))))*math.exp(-32*t)*min(1,i/20)*.85
        if name!='bed':v*=min(1,(lengths[name]-1-i)/96)
        out.append(pcm_encode(v))
    return bytes(out),rate

def song(index):
    rate=44100;beat=.625 if index==2 else .75;duration=16*beat
    left=array('i',[0])*round(duration*rate);right=array('i',[0])*len(left)
    roots=[48,53,45,55] if index==2 else [45,41,48,43]
    melody=[72,76,79,76,74,77,81,79,72,76,79,83,81,79,76,74] if index==2 else [76,72,69,67,72,69,65,64,74,71,67,64,71,67,62,67]
    def note(midi,start,length,gain,pan,plucked=False):
        freq=440*2**((midi-69)/12);begin=round(start*rate);count=min(round(length*rate),len(left)-begin)
        for i in range(count):
            t=i/rate;u=i/max(1,count-1)
            env=min(1,t/.015)*min(1,(1-u)*12)*(math.exp(-3*u) if plucked else .75+.25*math.sin(math.pi*u))
            v=math.sin(2*math.pi*freq*t)+.18*math.sin(2*math.pi*freq*2*t)
            sample=round(v*env*gain*10000)
            left[begin+i]+=sample*(255-pan)//255;right[begin+i]+=sample*pan//255
    for bar,root in enumerate(roots):
        for chord in [root,root+7,root+16]:note(chord,bar*4*beat,4*beat,.27,72 if chord==root else 180)
        for b in range(4):
            note(root-12,(bar*4+b)*beat,.8*beat,.35,128,True)
            note(melody[bar*4+b],(bar*4+b)*beat,1.15*beat,.64,96 if b&1 else 160,True)
            note(root+24+(7 if b&1 else 0),(bar*4+b+.5)*beat,.4*beat,.12,210,True)
    data=bytearray(88200*4) # Physical two-second pregap, skipped by INDEX 01.
    for i,(l,r) in enumerate(zip(left,right)):
        fade=min(1,i/(rate*.03),(len(left)-1-i)/(rate*.12))
        data+=struct.pack('<hh',max(-32768,min(32767,round(l*fade))),max(-32768,min(32767,round(r*fade))))
    data+=bytes((-len(data))%2352)
    return bytes(data)

def generate(root=ROOT):
    res=root/'res';build=root/'build';res.mkdir(exist_ok=True);build.mkdir(exist_ok=True)
    fingerprint=hashlib.sha256(Path(__file__).read_bytes()).hexdigest();stamp=build/'audio-source.json'
    if stamp.exists():
        old=json.loads(stamp.read_text())
        if old.get('source')==fingerprint and all((res/name).is_file() and hashlib.sha256((res/name).read_bytes()).hexdigest()==digest for name,digest in old.get('files',{}).items()) and (build/'audio_assets.h').exists() and hashlib.sha256((build/'audio_assets.h').read_bytes()).hexdigest()==old.get('header'):return
    manifest={'source':fingerprint,'files':{}};headers=[]
    for name in ['bed','bell','pluck','drum']:
        data,rate=pcm_sample(name);path=res/(name+'.pcm');path.write_bytes(data)
        headers.append(f'#define AUDIO_{name.upper()}_BYTES {len(data)}u\n')
        headers.append(f'#define AUDIO_{name.upper()}_STEP {(rate*2048*384+6250000)//12500000}u\n')
        headers.append(f'#define AUDIO_{name.upper()}_TICKS {(len(data)*300+rate-1)//rate+3}u\n')
        manifest['files'][path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
    for index in [2,3]:
        path=res/f'track{index:02}.wav'
        with wave.open(str(path),'wb') as out:out.setparams((2,2,44100,0,'NONE','not compressed'));out.writeframes(song(index))
        manifest['files'][path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
    (build/'audio_assets.h').write_text('/* Generated sample lengths, pitch steps and 300 Hz lifetimes. */\n'+''.join(headers))
    manifest['header']=hashlib.sha256((build/'audio_assets.h').read_bytes()).hexdigest()
    stamp.write_text(json.dumps(manifest,indent=2)+'\n')
if __name__=='__main__':generate()

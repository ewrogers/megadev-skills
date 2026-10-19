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

def generate(root=ROOT):
    res=root/'res';build=root/'build';res.mkdir(exist_ok=True);build.mkdir(exist_ok=True)
    headers=[]
    for name in ['bed','bell','pluck','drum']:
        data,rate=pcm_sample(name);(res/(name+'.pcm')).write_bytes(data)
        headers += [f'#define AUDIO_{name.upper()}_BYTES {len(data)}u\n',f'#define AUDIO_{name.upper()}_STEP {(rate*2048*384+6250000)//12500000}u\n',f'#define AUDIO_{name.upper()}_TICKS {(len(data)*300+rate-1)//rate+3}u\n']
    for name in ['welcome','listen']:
        with wave.open(str(res/('voice_'+name+'.wav')),'rb') as wav:
            if (wav.getnchannels(),wav.getsampwidth()) != (1,2):raise ValueError('Voice source must be mono PCM16')
            rate=wav.getframerate();samples=list(struct.unpack('<'+'h'*wav.getnframes(),wav.readframes(wav.getnframes())))
        # Offline integer interpolation; never transform samples during playback.
        count=len(samples)*8000//rate
        if not 1<=count<=12000:raise ValueError('Voice exceeds its bounded resident sample allowance')
        values=[]
        peak=max(max(abs(v) for v in samples),1)
        for i in range(count):
            at,frac=divmod(i*rate,8000)
            value=(samples[at]*(8000-frac)+samples[min(at+1,len(samples)-1)]*frac)//8000
            edge=min(i,count-1-i,64)
            value=value*100*edge//(peak*64)
            values.append((0x80|value) if value>=0 else -value)
        (res/('voice_'+name+'.pcm')).write_bytes(bytes(values))
        headers += [f'#define VOICE_{name.upper()}_BYTES {count}u\n',f'#define VOICE_{name.upper()}_TICKS {(count*300+7999)//8000+3}u\n']
    headers.append(f'#define VOICE_STEP {(8000*2048*384+6250000)//12500000}u\n')
    (build/'audio_assets.h').write_text('/* Generated resident PCM lengths and 300 Hz lifetimes. */\n'+''.join(headers))
if __name__=='__main__':generate()

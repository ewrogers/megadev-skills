#!/usr/bin/env python3
"""Create a mixed-mode CUE without replacing an existing different layout."""
import argparse
from pathlib import Path
import wave

def write_cue(image):
    image=Path(image)
    if not image.is_file() or image.stat().st_size%2048:raise ValueError('Expected a sector-aligned Sega CD data track')
    with image.open('rb') as source:
        if source.read(16)!=b'SEGADISCSYSTEM  ':raise ValueError('Invalid Sega CD boot signature')
    if any(c in image.name for c in ['"','\n','\r']):raise ValueError('Invalid image filename')
    lines=[f'FILE "{image.name}" BINARY','  TRACK 01 MODE1/2048','    INDEX 01 00:00:00']
    for track in [2,3]:
        path=image.parent/'res'/f'track{track:02}.wav'
        with wave.open(str(path),'rb') as audio:
            if (audio.getnchannels(),audio.getsampwidth(),audio.getframerate(),audio.getcomptype())!=(2,2,44100,'NONE'):raise ValueError('CD audio needs stereo 16-bit 44100 Hz PCM WAV')
            if audio.getnframes()%588 or audio.getnframes()<44100*6:raise ValueError('Audio must contain whole sectors and at least four seconds plus pregap')
            if audio.readframes(88200)!=bytes(88200*4):raise ValueError('Expected an actual two-second silent pregap')
        lines += [f'FILE "res/{path.name}" WAVE',f'  TRACK {track:02} AUDIO','    INDEX 00 00:00:00','    INDEX 01 00:02:00']
    output=image.with_suffix('.cue');text='\n'.join(lines)+'\n'
    if output.exists() and output.read_text()!=text:raise FileExistsError('Refusing to replace an existing different disc layout')
    output.write_text(text);return output
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('image',type=Path);args=parser.parse_args();print(write_cue(args.image))

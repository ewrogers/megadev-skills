#!/usr/bin/env python3
"""Render the actual C tilemaps and native patterns to PNG for layout review."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zlib

def preview(output):
    root=Path(__file__).resolve().parents[1]
    output=Path(output);output.mkdir(parents=True,exist_ok=False)
    cc=next((p for n in ('cc','clang','gcc') if (p:=shutil.which(n))),None)
    if not cc:raise RuntimeError('A host C compiler is required')
    with tempfile.TemporaryDirectory() as folder:
        src=Path(folder)/'preview.c';exe=Path(folder)/('preview'+('.exe' if os.name=='nt' else ''))
        # Windows stdout defaults to text mode and expands byte 0x0A to CR/LF.
        # A binary file preserves the exact map words on every supported host.
        raw=Path(folder)/'maps.bin'
        src.write_text('#include "ui.h"\n#include <stdio.h>\nint main(int argc,char **argv){if(argc!=2)return 1;FILE *out=fopen(argv[1],"wb");if(!out)return 1;UiModel m;UiCanvas c;ui_init(&m);for(unsigned i=0;i<8;i++){m.page=i;ui_draw(&c,&m,60);for(unsigned j=0;j<UI_CELLS;j++){fputc(c.cells[j]>>8,out);fputc(c.cells[j]&255,out);}}return fclose(out)!=0;}\n')
        subprocess.run([cc,'-std=c17','-I',str(root/'src'),str(root/'src/ui.c'),str(src),'-o',str(exe)],check=True,capture_output=True)
        subprocess.run([str(exe),str(raw)],check=True,capture_output=True)
        maps=raw.read_bytes()
        if len(maps)!=8*2240:raise ValueError('C preview returned an incomplete tilemap catalog')
    patterns=bytearray(0x1C8*32)
    for name,base in [('fonts',0x20),('heading',0xE0),('panels',0x170),('icons',0x180),('portrait',0x1A0),('backdrop',0x1C4)]:
        data=(root/'res'/f'{name}.bin').read_bytes();patterns[base*32:base*32+len(data)]=data
    words=struct.unpack('>64H',(root/'res/palette.bin').read_bytes())
    colors=[bytes(((v&14)*255//14,((v>>4)&14)*255//14,((v>>8)&14)*255//14)) for v in words]
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    def png(path,pixels):
        data=b''.join(b'\0'+bytes(row) for row in pixels)
        path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',320,224,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(data))+chunk(b'IEND',b''))
    for page,name in enumerate(('title','field','pack','character','settings','credits','pause','detail')):
        cells=struct.unpack('>1120H',maps[page*2240:(page+1)*2240]);pixels=[]
        for y in range(224):
            row=bytearray()
            for x in range(320):
                cell=cells[y//8*40+x//8];tile=cell&2047;pal=(cell>>13)&3
                value=patterns[tile*32+(y%8)*4+(x%8)//2];index=(value>>(0 if x&1 else 4))&15
                if not index:
                    tile=0x1C6+((x//8+y//8)&1);value=patterns[tile*32+(y%8)*4+(x%8)//2];index=(value>>(0 if x&1 else 4))&15;pal=0
                row+=colors[pal*16+index]
            pixels.append(row)
        png(output/(name+'.png'),pixels)
    print(output)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);a=p.parse_args();preview(a.output)

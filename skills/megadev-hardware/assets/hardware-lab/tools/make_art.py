#!/usr/bin/env python3
"""Original native VDP tiles; stable source/configuration identity, no SDK APIs."""
import hashlib
from pathlib import Path
import sys
from hardware_glyphs import rows

root = Path(__file__).resolve().parents[1]
digest = hashlib.sha256()
for path in sorted([root/'Makefile', *root.glob('src/*'), *root.glob('tools/*.py')]):
    if path.is_file():
        digest.update(path.relative_to(root).as_posix().encode()+b'\0'+path.read_bytes())
digest.update(' '.join(sys.argv[1:]).encode())
tiles = [[0]*64]  # zero is transparent
for code in range(32, 128):
    tiles.append([1 if x in range(1,6) and rows(chr(code))[y] & (1 << (5-x)) else 0
                  for y in range(8) for x in range(8)])
tiles += [[6]*64, [2 if y == 0 else 6 for y in range(8) for x in range(8)],
          [2 if x == 0 else 6 for y in range(8) for x in range(8)]]
# Large optical disc: a tiled pixel graphic, with a hub and cyan reflections.
for ty in range(6):
    for tx in range(6):
        pixels=[]
        for y in range(8):
            for x in range(8):
                dx,dy=tx*8+x-23,ty*8+y-23; d=dx*dx+dy*dy
                pixels.append(0 if d>529 or d<25 else (2 if d<70 or abs(dx+dy)<5 else (3 if dx>dy else 1)))
        tiles.append(pixels)
# Two-by-two title glyphs use the same original font.
for char in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ ':
    scaled=[[1 if 2<=x<12 and rows(char)[y//2] & (1 << (5-x//2)) else 0 for x in range(16)] for y in range(16)]
    for ty in range(2):
        for tx in range(2):
            tiles.append([scaled[ty*8+y][tx*8+x] for y in range(8) for x in range(8)])
words=[]
for tile in tiles:
    for i in range(0,64,4):
        words.append(sum(tile[i+j] << (12-j*4) for j in range(4)))
out = root/'build/art.h'
out.parent.mkdir(exist_ok=True)
text = '#define LAB_BUILD_ID "'+digest.hexdigest()[:8].upper()+'"\n'
text += 'static const u16 lab_tiles[] = {\n'
text += '\n'.join(','.join('0x%04x'%v for v in words[i:i+16])+',' for i in range(0,len(words),16))+'\n};\n'
if not out.exists() or out.read_text() != text:
    out.write_text(text)

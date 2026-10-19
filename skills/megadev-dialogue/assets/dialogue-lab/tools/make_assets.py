#!/usr/bin/env python3
"""Encode original native-resolution UI art; no resizing or external libraries."""
from pathlib import Path
import struct
from glyphs import GLYPHS

ROOT = Path(__file__).resolve().parents[1]
GLYPHS.update({
    ' ': ['00000'] * 7,
    ',': ['00000'] * 5 + ['00100','01000'],
    "'": ['00100','00100'] + ['00000'] * 5,
    '?': ['01110','10001','00001','00010','00100','00000','00100'],
    '#': ['01010','11111','01010','01010','11111','01010','00000'],
    '.': ['00000'] * 5 + ['00100','00100'],
    '!': ['00100'] * 5 + ['00000','00100'],
    '=': ['00000','00000','11111','00000','11111','00000','00000'],
    '<': ['00001','00010','00100','01000','00100','00010','00001'],
})

def canvas(w, h, color=0):
    return [[color] * w for _ in range(h)]

def tiles(image):
    """Row-major tiles; each tile uses four packed nibbles per scanline."""
    h, w = len(image), len(image[0])
    assert w % 8 == h % 8 == 0
    result = bytearray()
    for ty in range(0, h, 8):
        for tx in range(0, w, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    a, b = image[ty+y][tx+x:tx+x+2]
                    assert 0 <= a < 16 and 0 <= b < 16
                    result.append(a * 16 + b)
    return result

def glyph(ch, scale=1, classic=False):
    image = canvas(8*scale, 8*scale, 1)
    rows = GLYPHS.get(ch.upper(), GLYPHS['?'])
    for y, row in enumerate(rows):
        for x, bit in enumerate(row):
            if bit == '1':
                for dy in range(scale):
                    for dx in range(scale):
                        image[y*scale+dy][(x+1)*scale+dx] = 3 if classic and y < 2 else 2
    return image

def panels():
    result = bytearray()
    for row in range(3):
        for col in range(3):
            image = canvas(8, 8, 1)
            for y in range(8):
                for x in range(8):
                    top, bottom = row == 0 and y < 2, row == 2 and y >= 6
                    left, right = col == 0 and x < 2, col == 2 and x >= 6
                    if top or left: image[y][x] = 3
                    if bottom or right: image[y][x] = 4
            result += tiles(image)
    for frame in range(2):
        image = canvas(8, 8, 1)
        for y in range(1, 7):
            for x in range(1, 7):
                if abs(y-3) + abs(x-3) < 3: image[y][x] = 2 if frame else 3
        result += tiles(image)
    for filled in (True, False):
        image = canvas(8, 8, 1)
        for y in range(2, 6):
            for x in range(1, 7): image[y][x] = (5 if y < 4 else 6) if filled else 4
        result += tiles(image)
    return result

def icon(n):
    p = canvas(16, 16)
    for y in range(16):
        for x in range(16):
            if n == 0:  # lantern
                if 5 <= x <= 10 and 5 <= y <= 12: p[y][x] = 3 if x in (5,10) or y in (5,12) else 4
                if 6 <= x <= 9 and y in (2,3,14): p[y][x] = 2
            elif n == 1:  # key
                if (x-5)**2+(y-5)**2 <= 13: p[y][x] = 3 if (x-5)**2+(y-5)**2 > 3 else 0
                if x in (7,8) and 7 <= y <= 14 or y == 12 and 8 <= x <= 11: p[y][x] = 4
            elif n == 2:  # map
                if 2 <= x <= 13 and 3 <= y <= 12: p[y][x] = 3 if x in (2,13) or y in (3,12) else 8
                if 4 <= x <= 11 and (x+y)%7 == 0 and 5 <= y <= 10: p[y][x] = 6
            elif n == 3:  # apple
                if (x-7)**2+(y-9)**2 <= 32: p[y][x] = 5 if x > 6 else 10
                if 5 <= x <= 9 and y in (2,3): p[y][x] = 6
            elif n == 4:  # ink
                if 4 <= x <= 11 and 6 <= y <= 13: p[y][x] = 7 if x > 5 else 9
                if 6 <= x <= 9 and 2 <= y <= 5: p[y][x] = 3
            elif n == 5:  # compass
                d = abs(x-7)+abs(y-7)
                if d <= 7: p[y][x] = 3 if d >= 6 else 7
                if abs(x-7) <= 1 and 3 <= y <= 11: p[y][x] = 2 if y < 7 else 5
            elif n == 6:  # coin
                d = (x-7)**2 + (y-7)**2
                if d <= 38: p[y][x] = 3 if d > 21 else 4
                if x == 7 and 4 <= y <= 10: p[y][x] = 2
            else:  # tile-based courier, deliberately not a sprite engine
                if 5 <= x <= 10 and 2 <= y <= 6: p[y][x] = 8 if y > 3 else 3
                if 4 <= x <= 11 and 7 <= y <= 12: p[y][x] = 9 if x < 7 else 7
                if x in (5,10) and y in (13,14): p[y][x] = 3
    return p

def portrait(speaker=0, pose=0):
    p = canvas(48, 48, 7)
    for y in range(48):
        for x in range(48):
            if x < 2 or x >= 46 or y < 2 or y >= 46: p[y][x] = 3
            if (x-24)**2 + (y-19)**2 < 120: p[y][x] = 8
            if 13 <= x <= 34 and 7 <= y <= 15 or 12 <= x <= 16 and 12 <= y <= 28: p[y][x] = 3 if not speaker else 9
            if y >= 32 and abs(x-24) < y-20: p[y][x] = 9 if x < 23 else 6
            if x in (20,29) and y in (19,20): p[y][x] = 1 if pose != 2 else 3
            if 22 <= x <= 27 and y in ((26,27) if pose == 1 else (26,)): p[y][x] = 5
    return p

def build():
    out = ROOT / 'res'
    out.mkdir(exist_ok=True)
    fonts = b''.join(tiles(glyph(chr(n), classic=classic)) for classic in (False, True) for n in range(32,128))
    heading = b''.join(tiles(glyph(ch, 2, True)) for ch in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789')
    backdrop = bytearray()
    for n in range(4):
        image = canvas(8,8,1)
        for y in range(8):
            for x in range(8):
                if (x+y+n*3)%11 == 0: image[y][x] = 2
        backdrop += tiles(image)
    colors = [
        [0x0200,0x0420,0x0642,0x0864],
        [0,0x0200,0x0CEE,0x068A,0x0422,0x06C6,0x0282],
        [0,0x0422,0x0AEE,0x08CE,0x0644,0x06C6,0x0282],
        [0,0x0200,0x0EEE,0x046A,0x08CE,0x046C,0x0686,0x0422,0x08AC,0x0CA6,0x068E],
    ]
    palette = b''.join(struct.pack('>H', value) for bank in colors for value in bank+[0]*(16-len(bank)))
    assets = {'fonts.bin':fonts,'heading.bin':heading,'panels.bin':panels(),
              'icons.bin':b''.join(tiles(icon(n)) for n in range(8)),
              'portrait.bin':b''.join(tiles(portrait(speaker,pose)) for speaker in range(2) for pose in range(3)),'backdrop.bin':backdrop,'palette.bin':palette}
    for name, data in assets.items(): (out/name).write_bytes(data)
    print('Generated native UI patterns and four palette banks.')

if __name__ == '__main__': build()

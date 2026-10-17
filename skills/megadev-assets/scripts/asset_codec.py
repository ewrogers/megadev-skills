"""Small teaching format for native Mega Drive patterns; not a Megadev ABI."""
import struct
import zlib
HEADER = struct.Struct('>4s6H4I')
LEVELS = tuple(round(i * 255 / 7) for i in range(8))


def cram(rgb, snap=False):
    if len(rgb) != 3 or any(type(c) is not int or not 0 <= c <= 255 for c in rgb):
        raise ValueError('Expected three 8-bit RGB channels')
    values = [min(range(8), key=lambda i: abs(LEVELS[i] - c)) for c in rgb]
    if not snap and tuple(LEVELS[i] for i in values) != tuple(rgb):
        raise ValueError('Color is off the 3-bit channel grid; use --snap explicitly or edit the source')
    return values[0] << 1 | values[1] << 5 | values[2] << 9


def rgb(word):
    if word & 0xF111:
        raise ValueError('Illegal CRAM bits')
    return tuple(LEVELS[(word >> shift) & 7] for shift in (1, 5, 9))


def tile_bytes(pixels):
    if len(pixels) != 64 or any(type(p) is not int or not 0 <= p < 16 for p in pixels):
        raise ValueError('An 8x8 tile needs 64 palette indices, each 0..15')
    return bytes(pixels[i] << 4 | pixels[i + 1] for i in range(0, 64, 2))


def tile_pixels(data):
    return [n for b in data for n in (b >> 4, b & 15)]


def flipped(pixels, h=False, v=False):
    return [pixels[(7-y if v else y)*8 + (7-x if h else x)] for y in range(8) for x in range(8)]


def pack(indices, width, height, palette, frame=None):
    if width <= 0 or height <= 0 or width % 8 or height % 8 or len(indices) != width * height:
        raise ValueError('Images must have positive dimensions divisible by 8; no resizing is performed')
    if len(palette) != 16:
        raise ValueError('A palette needs exactly 16 entries')
    for color in palette:
        rgb(color)
    patterns, mapping = [], []
    def get_tile(x, y):
        return [indices[(y+dy)*width+x+dx] for dy in range(8) for dx in range(8)]
    if frame:
        fw, fh = frame
        if fw not in (8,16,24,32) or fh not in (8,16,24,32) or width % fw or height % fh:
            raise ValueError('Sprite frames must be 8..32 pixels per axis in 8-pixel steps and divide the sheet')
        w, h, kind = fw//8, fh//8, 1
        entries = (width//fw)*(height//fh)
        # Frame cells in row order; hardware sprites consume tiles by columns.
        for fy in range(0,height,fh):
            for fx in range(0,width,fw):
                for tx in range(w):
                    for ty in range(h):
                        patterns.append(tile_bytes(get_tile(fx+tx*8,fy+ty*8)))
    else:
        w, h, kind, entries = width//8, height//8, 0, (width//8)*(height//8)
        if w > 64 or h > 32:
            raise ValueError('This teaching background format supports up to 64x32 cells')
        seen = {}
        for y in range(0,height,8):
            for x in range(0,width,8):
                p = get_tile(x,y)
                data, flags = min((tile_bytes(flipped(p,hf,vf)), (0x800 if hf else 0)|(0x1000 if vf else 0))
                                  for hf,vf in [(False,False),(True,False),(False,True),(True,True)])
                if data not in seen:
                    seen[data] = len(patterns); patterns.append(data)
                mapping.append(seen[data]|flags)
    if not 0 < len(patterns) <= 2048 or entries > 65535:
        raise ValueError('Asset exceeds this format\'s 11-bit pattern indices or entry count')
    tiles = b''.join(patterns)
    payload = struct.pack('>16H',*palette)+tiles+b''.join(struct.pack('>H',m) for m in mapping)
    return HEADER.pack(b'MDA1',1,kind,w,h,len(patterns),entries,len(tiles),32+len(payload),zlib.crc32(payload),0)+payload


def inspect(data):
    if len(data) < 64:
        raise ValueError('Truncated asset')
    magic, version, kind, w, h, count, entries, size, total, crc, reserved = HEADER.unpack_from(data)
    if magic != b'MDA1' or version != 1 or kind not in (0,1) or reserved or total != len(data):
        raise ValueError('Invalid asset header or size')
    if not w or not h or not 0 < count <= 2048 or size != count*32:
        raise ValueError('Invalid pattern geometry')
    if kind == 0 and (w > 64 or h > 32 or entries != w*h):
        raise ValueError('Invalid background map')
    if kind == 1 and (w > 4 or h > 4 or not entries or count != w*h*entries):
        raise ValueError('Invalid sprite sheet')
    if total != 64+size+(entries*2 if kind == 0 else 0):
        raise ValueError('Invalid section lengths')
    if zlib.crc32(data[32:]) != crc:
        raise ValueError('Asset CRC mismatch')
    palette = list(struct.unpack_from('>16H',data,32))
    for color in palette: rgb(color)
    mapping = list(struct.unpack_from('>'+str(entries)+'H',data,64+size)) if kind == 0 else []
    if any(m & 0xE000 or (m & 0x7FF) >= count for m in mapping):
        raise ValueError('Invalid relative tile reference or embedded palette/priority bits')
    return {'kind':kind,'width_tiles':w,'height_tiles':h,'tiles':count,'entries':entries,
            'tile_bytes':size,'total_bytes':total,'crc32':crc,'palette':palette,'mapping':mapping}


def unpack(data, columns=None):
    info = inspect(data); w,h = info['width_tiles'],info['height_tiles']
    tiles = [tile_pixels(data[64+i*32:96+i*32]) for i in range(info['tiles'])]
    if not info['kind']:
        width,height=w*8,h*8; out=[0]*(width*height)
        cells=[(i%w,i//w,flipped(tiles[m&0x7FF],bool(m&0x800),bool(m&0x1000))) for i,m in enumerate(info['mapping'])]
    else:
        columns = info['entries'] if columns is None else columns
        if type(columns) is not int or columns <= 0 or info['entries'] % columns:
            raise ValueError('Invalid preview frame columns')
        width,height=columns*w*8,(info['entries']//columns)*h*8;out=[0]*(width*height);cells=[]
        for frame in range(info['entries']):
            for x in range(w):
                for y in range(h):
                    cells.append(((frame%columns)*w+x,(frame//columns)*h+y,tiles[frame*w*h+x*h+y]))
    for x,y,pixels in cells:
        for dy in range(8):out[(y*8+dy)*width+x*8:(y*8+dy)*width+x*8+8]=pixels[dy*8:dy*8+8]
    return width,height,out

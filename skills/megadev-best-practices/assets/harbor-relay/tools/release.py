#!/usr/bin/env python3
"""Validate and package the documented Megadev multi-file mixed-mode profile."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import stat
import struct
import sys
import tempfile
import wave
import zipfile
import zlib

SECTOR = 2048
MAX_BYTES = 1024 * 1024 * 1024  # Tool safety bound, not a claimed CD capacity.
SIGNATURES = {'US': (1412, 0xF361AB57), 'JP': (340, 0x4571F623), 'EU': (1390, 0x6FFB4732)}
MANIFEST = 'RELEASE.json'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def digest(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def portable(name):
    require(isinstance(name, str) and 0 < len(name) <= 240, 'Invalid relative filename')
    require(not any(c in name for c in '\\:"\r\n\x00'), 'Nonportable filename')
    p = PurePosixPath(name)
    require(not p.is_absolute() and str(p) == name and all(x not in ('.', '..') for x in p.parts), 'Unsafe relative path')
    for part in p.parts:
        require(re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_. -]*', part) and not part.endswith((' ', '.')), 'Nonportable path component')
        require(part.split('.')[0].upper() not in {'CON', 'PRN', 'AUX', 'NUL', *('COM'+str(i) for i in range(1,10)), *('LPT'+str(i) for i in range(1,10))}, 'Reserved device filename')
    return name


def read_file(root, name):
    portable(name)
    root = Path(root).resolve()
    path = root
    for part in PurePosixPath(name).parts:
        path = path / part
        require(not path.is_symlink(), 'Symlinks are not release inputs')
    require(path.is_file() and path.stat().st_size <= MAX_BYTES, 'Missing or oversized file: '+name)
    return path.read_bytes()


def both(data, offset, width):
    require(offset + width*2 <= len(data), 'Truncated ISO numeric field')
    a = int.from_bytes(data[offset:offset+width], 'little')
    require(a == int.from_bytes(data[offset+width:offset+width*2], 'big'), 'ISO endian copies disagree')
    return a


def inspect_iso(data, region):
    require(region in SIGNATURES, 'Specify US, JP or EU boot region')
    require(len(data) >= 19*SECTOR and len(data) % SECTOR == 0 and data[:16] == b'SEGADISCSYSTEM  ', 'Invalid Sega CD ISO signature or sector size')
    length, crc = SIGNATURES[region]
    require(zlib.crc32(data[0x200:0x200+length]) == crc, 'Regional security block mismatch')
    fields = struct.unpack('>8I', data[0x30:0x50])
    require(fields[:2] == (0x800, 0x800) and fields[2:4] == (0,0) and fields[4] == 0x1000 and fields[6:] == (0,0), 'Unsupported Megadev IP/SP boot layout')
    require(0 < fields[5] <= 0x7000 and fields[5] % 2 == 0, 'Sub boot payload exceeds boot area')
    pvd = data[16*SECTOR:17*SECTOR]
    require(pvd[:7] == b'\x01CD001\x01', 'Missing primary ISO descriptor')
    require(both(pvd,80,4) == len(data)//SECTOR and both(pvd,128,2) == SECTOR, 'ISO volume size or block size mismatch')
    require(both(pvd,120,2) == both(pvd,124,2) == 1, 'Multi-volume ISO unsupported')
    require(data[17*SECTOR:17*SECTOR+7] == b'\xffCD001\x01', 'Expected descriptor terminator; extensions unsupported')
    files, extents, visited = {}, [], set()

    def walk(record, prefix, depth):
        require(depth <= 8 and len(record) >= 34 and record[0] == len(record), 'Invalid ISO directory record')
        require(record[1] == record[26] == record[27] == 0 and record[25] in (0,2) and both(record,28,2) == 1, 'Unsupported ISO extent flags/interleave')
        sector, size = both(record,2,4), both(record,10,4)
        start = sector*SECTOR
        require(sector >= 18 and start+size <= len(data), 'ISO extent outside data area')
        if record[25] == 2:
            require(size > 0 and size % SECTOR == 0 and sector not in visited, 'Invalid/cyclic/aliased ISO directory')
            visited.add(sector)
        if size:
            end = start + ((size+SECTOR-1)//SECTOR)*SECTOR
            require(not any(start < b and a < end for a,b in extents), 'Overlapping ISO extents')
            extents.append((start,end))
        if record[25] == 2:
            block, offset = data[start:start+size], 0
            while offset < size:
                n = block[offset]
                if not n:
                    offset = (offset//SECTOR+1)*SECTOR
                    continue
                require(n >= 34 and offset+n <= size and offset%SECTOR+n <= SECTOR, 'Directory record crosses sector')
                child = block[offset:offset+n]
                name_len = child[32]
                require(33+name_len <= n and name_len > 0, 'Invalid ISO name length')
                raw = child[33:33+name_len]
                if raw not in (b'\0', b'\1'):
                    name = raw.decode('ascii')
                    directory = bool(child[25] & 2)
                    require(re.fullmatch(r'[A-Z0-9_]{1,8}' if directory else r'[A-Z0-9_]{1,8}(?:\.[A-Z0-9_]{1,3})?;1', name), 'Unsupported ISO level-1 name/version')
                    name = name.removesuffix(';1')
                    walk(child, prefix+'/'+name if prefix else name, depth+1)
                offset += n
        else:
            require(prefix not in files, 'Duplicate ISO path')
            files[prefix] = digest(data[start:start+size])
    n = pvd[156]
    root = pvd[156:156+n]
    require(len(root) >= 34 and root[25] == 2 and root[32:34] == b'\1\0', 'Invalid ISO root directory')
    walk(root, '', 0)
    require(files, 'Empty data disc')
    return {'boot': digest(data[:32768]), 'sp_bytes': fields[5], 'files': files}


def cue_layout(text):
    tracks, current, pending = [], None, None
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith('REM '):
            continue
        m = re.fullmatch(r'FILE "([^"]+)" (BINARY|WAVE)', line)
        if m:
            require(pending is None, 'FILE without a track')
            pending = (portable(m[1]), m[2]); current = None
            continue
        m = re.fullmatch(r'TRACK (\d{2}) (MODE1/2048|AUDIO)', line)
        if m:
            require(pending is not None, 'Only one track per FILE is supported')
            number = int(m[1])
            require(number == len(tracks)+1 and number <= 99, 'Track numbers must be consecutive from 01')
            require((number == 1 and m[2] == 'MODE1/2048' and pending[1] == 'BINARY') or (number > 1 and m[2] == 'AUDIO' and pending[1] == 'WAVE'), 'Data track must be first')
            current = {'number': number, 'file': pending[0], 'format': pending[1], 'mode': m[2], 'index': {}}
            tracks.append(current); pending = None
            continue
        m = re.fullmatch(r'INDEX (00|01) (\d{2}):([0-5]\d):([0-7]\d)', line)
        if m:
            require(current is not None and int(m[4]) < 75 and m[1] not in current['index'], 'Invalid/duplicate CUE index')
            frames = (int(m[2])*60+int(m[3]))*75+int(m[4])
            require(not current['index'] or m[1] == '01', 'CUE indices out of order')
            current['index'][m[1]] = frames
            continue
        raise ValueError('Unsupported CUE directive: '+line)
    require(tracks and pending is None and all('01' in t['index'] for t in tracks), 'Incomplete CUE')
    names = [t['file'].casefold() for t in tracks]
    require(len(set(names)) == len(names), 'Track files alias on case-insensitive hosts')
    require(tracks[0]['index'] == {'01':0}, 'Data track must start at file offset zero')
    for t in tracks[1:]:
        require(t['index'].get('00',0) == 0 and t['index']['01'] >= t['index'].get('00',0), 'Invalid audio pregap indices')
    return tracks


def inspect(cue, region):
    cue = Path(cue).absolute()
    raw = read_file(cue.parent, cue.name)
    require(len(raw) <= 65536, 'Oversized CUE')
    tracks = cue_layout(raw.decode('utf-8'))
    artifacts = {cue.name: digest(raw)}
    require(cue.name.casefold() not in {t['file'].casefold() for t in tracks}, 'CUE aliases a track')
    iso = None
    for t in tracks:
        data = read_file(cue.parent,t['file']); artifacts[t['file']] = digest(data)
        require(sum(item['bytes'] for item in artifacts.values()) <= MAX_BYTES, 'Disc exceeds tool size bound')
        if t['number'] == 1:
            iso = inspect_iso(data, region); t['sectors'] = len(data)//SECTOR
        else:
            import io
            with wave.open(io.BytesIO(data),'rb') as audio:
                require((audio.getnchannels(),audio.getsampwidth(),audio.getframerate(),audio.getcomptype()) == (2,2,44100,'NONE'), 'CD audio needs stereo signed 16-bit 44100 Hz PCM WAV')
                frames = audio.getnframes()
                pcm = audio.readframes(frames)
                require(len(pcm) == frames*4, 'Truncated WAV PCM')
            require(frames%588 == 0 and frames//588-t['index']['01'] >= 300, 'Audio needs whole sectors and at least four seconds of program')
            t['sectors'] = frames//588
            t['pcm'] = digest(pcm)
    return {'cue': cue.name, 'region': region, 'tracks': tracks, 'iso': iso, 'artifacts': artifacts}


def contract(cue, region, video, payloads, boot):
    require(video in ('NTSC','PAL'), 'Video must be NTSC or PAL; it is not inferred from boot region')
    report = inspect(cue,region)
    payloads, boot = Path(payloads), Path(boot)
    expected = {}
    for p in sorted(payloads.rglob('*')):
        require(not p.is_symlink(), 'Symlink in payload directory')
        if p.is_file():
            name = str(p.relative_to(payloads)).replace('\\','/').upper()
            require(name not in expected, 'Case-alias in compiled payloads')
            expected[name] = digest(p.read_bytes())
    require(expected == report['iso']['files'], 'Disc files differ from compiled payload directory')
    ip, sp = read_file(boot,'ip.bin'), read_file(boot,'sp.bin')
    require(0 < len(ip) <= 0xE00 and len(sp) == report['iso']['sp_bytes'], 'Compiled boot payload sizes disagree')
    data = read_file(Path(cue).parent,report['tracks'][0]['file'])
    require(data[0x200:0x200+len(ip)] == ip and data[0x1000:0x1000+len(sp)] == sp, 'Boot payloads differ from build')
    return {'schema':1, 'region':region, 'video':video, 'disc':report, 'compiled_boot':{'ip':digest(ip),'sp':digest(sp)}}


def validate(cue, expected):
    require(set(expected) == {'schema','region','video','disc','compiled_boot'} and type(expected['schema']) is int and expected['schema'] == 1 and expected['video'] in ('PAL','NTSC'), 'Invalid release contract')
    actual = inspect(cue, expected['region'])
    # Exact file hashes also pin boot payloads, audio and pregaps after approval.
    require(actual == expected['disc'], 'Disc or track contents/layout differ from approved contract')
    data = read_file(Path(cue).parent,actual['tracks'][0]['file'])
    boot = expected['compiled_boot']
    require(set(boot) == {'ip','sp'}, 'Missing compiled boot provenance')
    for name,offset,maximum in [('ip',0x200,0xE00),('sp',0x1000,0x7000)]:
        n = boot[name]['bytes']
        require(type(n) is int and 0 < n <= maximum and digest(data[offset:offset+n]) == boot[name], 'Invalid compiled boot digest')
    require(boot['sp']['bytes'] == actual['iso']['sp_bytes'], 'SP length differs from contract')
    return actual


def metadata(value):
    require(isinstance(value,dict) and set(value) == {'source_commit','sdk_commit','compiler','container_reference'}, 'Provenance needs source_commit, sdk_commit, compiler and container_reference')
    require(all(isinstance(x,str) and 0 < len(x) <= 300 and not any(ord(c)<32 for c in x) for x in value.values()), 'Invalid provenance text')
    require(all(re.fullmatch('[0-9a-f]{40}',value[x]) for x in ('source_commit','sdk_commit')), 'Record full source and SDK Git commits')
    return value


def encoded(value):
    return (json.dumps(value,sort_keys=True,indent=2)+'\n').encode()


def package(cue, expected, provenance, output, include=()):
    report = validate(cue,expected)
    root = Path(cue).parent
    names = list(report['artifacts']) + [portable(n) for n in include]
    require(len({n.casefold() for n in names+[MANIFEST]}) == len(names)+1, 'Duplicate/reserved package name')
    require(len(names)+1 <= 10000, 'Too many package files')
    content = {name:read_file(root,name) for name in names}
    require(sum(map(len,content.values())) <= MAX_BYTES, 'Package exceeds tool size bound')
    manifest = {'schema':1,'contract':expected,'provenance':metadata(provenance),'files':{n:digest(d) for n,d in content.items()}}
    content[MANIFEST] = encoded(manifest)
    require(len(content[MANIFEST]) <= 65536, 'Release manifest exceeds tool size bound')
    # Exclusive output: never replace an approved release by accident.
    output = Path(output)
    with output.open('xb') as stream:
        try:
            with zipfile.ZipFile(stream,'w',compression=zipfile.ZIP_STORED) as archive:
                for name in sorted(content):
                    info = zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3
                    info.external_attr=(stat.S_IFREG|0o644)<<16
                    archive.writestr(info,content[name])
        except Exception:
            stream.close(); output.unlink(); raise
    return manifest


def verify(archive_path):
    with zipfile.ZipFile(archive_path) as archive, tempfile.TemporaryDirectory(prefix='megadev release ') as folder:
        infos=archive.infolist(); names=[portable(i.filename) for i in infos]
        require(len({n.casefold() for n in names}) == len(names) and MANIFEST in names, 'Duplicate package member or missing manifest')
        require(sum(i.file_size for i in infos) <= MAX_BYTES+65536 and len(infos) <= 10000, 'Oversized package')
        require(all(i.compress_type == zipfile.ZIP_STORED and not i.is_dir() and stat.S_IFMT(i.external_attr>>16) in (0,stat.S_IFREG) for i in infos), 'Unsupported package member type/compression')
        require(archive.getinfo(MANIFEST).file_size <= 65536, 'Oversized release manifest')
        manifest=json.loads(archive.read(MANIFEST))
        require(set(manifest) == {'schema','contract','provenance','files'} and type(manifest['schema']) is int and manifest['schema'] == 1, 'Invalid package manifest')
        metadata(manifest['provenance'])
        require(set(names) == set(manifest['files'])|{MANIFEST}, 'Package files differ from manifest')
        root=Path(folder)
        for name in manifest['files']:
            data=archive.read(name)
            require(digest(data) == manifest['files'][name], 'Package hash mismatch: '+name)
            path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        report=validate(root/portable(manifest['contract']['disc']['cue']),manifest['contract'])
        require(set(report['artifacts']) <= manifest['files'].keys(), 'Incomplete disc package')
        return manifest


def load(path):
    return json.loads(Path(path).read_text())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='command',required=True)
    p=sub.add_parser('inspect');p.add_argument('cue',type=Path);p.add_argument('--region',required=True,choices=SIGNATURES)
    p=sub.add_parser('contract');p.add_argument('cue',type=Path);p.add_argument('--region',required=True,choices=SIGNATURES);p.add_argument('--video',required=True,choices=['NTSC','PAL']);p.add_argument('--payloads',required=True,type=Path);p.add_argument('--boot',required=True,type=Path);p.add_argument('--output',required=True,type=Path)
    p=sub.add_parser('validate');p.add_argument('cue',type=Path);p.add_argument('--contract',required=True,type=Path)
    p=sub.add_parser('package');p.add_argument('cue',type=Path);p.add_argument('--contract',required=True,type=Path);p.add_argument('--provenance',required=True,type=Path);p.add_argument('--output',required=True,type=Path);p.add_argument('--include',action='append',default=[])
    p=sub.add_parser('verify');p.add_argument('archive',type=Path)
    a=parser.parse_args()
    try:
        if a.command=='inspect': result=inspect(a.cue,a.region)
        elif a.command=='contract':
            result=contract(a.cue,a.region,a.video,a.payloads,a.boot)
            with a.output.open('x') as f:f.write(encoded(result).decode())
        elif a.command=='validate':result=validate(a.cue,load(a.contract))
        elif a.command=='package':result=package(a.cue,load(a.contract),load(a.provenance),a.output,a.include)
        else:result=verify(a.archive)
        print(json.dumps(result,sort_keys=True,indent=2))
    except (ValueError,OSError,KeyError,TypeError,UnicodeError,EOFError,wave.Error,zipfile.BadZipFile) as e:
        print('Release rejected: '+str(e),file=sys.stderr);return 2
    return 0


if __name__=='__main__':
    raise SystemExit(main())

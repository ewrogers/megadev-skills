"""Snapshot Megadev ISO/CUE+WAV output before any hardware operation.

CUE subset matches megadev-release's authored layout: one MODE1/2048 data
file followed by optional separate PCM WAV tracks. This is not a rip converter.
"""
import contextlib
import hashlib
from pathlib import Path
import re
import tempfile
import wave

MAX_BYTES = 1024 * 1024 * 1024


def require(condition, message):
    if not condition:
        raise ValueError(message)


def portable(name):
    require(name and all(32 <= ord(c) <= 126 and c not in '\\/:*?"<>|' for c in name)
            and name not in ('.', '..') and not name.endswith((' ', '.')),
            'Disc files need flat, printable ASCII FAT filenames')
    require(len(name) <= 120, 'Disc filename exceeds 120 bytes')
    require(name.casefold() != 'bios.bin', 'BIOS files are not deployed by this helper')
    return name


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
            require(number == len(tracks)+1 and number <= 99, 'Tracks must be consecutive from 01')
            require((number == 1 and m[2] == 'MODE1/2048' and pending[1] == 'BINARY') or
                    (number > 1 and m[2] == 'AUDIO' and pending[1] == 'WAVE'),
                    'Use MODE1/2048 BINARY first, then optional AUDIO WAVE files')
            current = {'file': pending[0], 'index': {}}
            tracks.append(current); pending = None
            continue
        m = re.fullmatch(r'INDEX (00|01) (\d{2}):([0-5]\d):([0-7]\d)', line)
        if m:
            require(current is not None and int(m[4]) < 75 and m[1] not in current['index'], 'Invalid CUE index')
            require(not current['index'] or m[1] == '01', 'CUE indices out of order')
            current['index'][m[1]] = (int(m[2])*60+int(m[3]))*75+int(m[4])
            continue
        raise ValueError('Unsupported CUE directive: '+line)
    require(tracks and pending is None and all('01' in t['index'] for t in tracks), 'Incomplete CUE')
    require(tracks[0]['index'] == {'01': 0}, 'Data track must start at zero')
    names = [t['file'].casefold() for t in tracks]
    require(len(set(names)) == len(names), 'Track files alias on FAT')
    for t in tracks[1:]:
        require(t['index'].get('00', 0) == 0 and t['index']['01'] >= 0, 'Invalid audio pregap')
    return tracks


@contextlib.contextmanager
def prepare_disc(image):
    """Yield immutable temporary snapshots, CUE entry point, and file hashes."""
    image = Path(image).absolute()
    portable(image.name)
    require(not image.is_symlink() and image.is_file(), 'Disc input must be a regular file, not a symlink')
    suffix = image.suffix.lower()
    require(suffix in ('.iso', '.cue'), 'Deploy a Megadev .iso or authored .cue')
    if suffix == '.iso':
        entry = portable(image.stem + '.cue')
        cue = f'FILE "{image.name}" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n'.encode('ascii')
    else:
        require(image.stat().st_size <= 65536, 'CUE exceeds 64 KiB')
        entry = image.name
        with image.open('rb') as stream:
            cue = stream.read(65537)
        require(len(cue) <= 65536, 'CUE exceeds 64 KiB')
    tracks = cue_layout(cue.decode('ascii'))
    require(entry.casefold() not in {t['file'].casefold() for t in tracks}, 'CUE aliases a track')
    with tempfile.TemporaryDirectory(prefix='megadev-disc-') as temporary:
        root = Path(temporary)
        files, metadata, total = {}, {}, len(cue)
        for i, track in enumerate(tracks):
            name = track['file']; source = image.parent / name
            require(not source.is_symlink() and source.is_file(), 'Missing/unsafe track: '+name)
            size = source.stat().st_size
            require(0 < size <= MAX_BYTES and total + size <= MAX_BYTES, 'Disc exceeds 1 GiB tool bound or is empty')
            dest = root / name
            digest = hashlib.sha256(); copied = 0
            with source.open('rb') as src, dest.open('xb') as out:
                while block := src.read(1024 * 1024):
                    copied += len(block)
                    require(copied <= size, 'Track changed while snapshotting')
                    out.write(block); digest.update(block)
            require(copied == size, 'Track changed while snapshotting')
            total += size
            if i == 0:
                with dest.open('rb') as stream:
                    header = stream.read(17*2048)
                require(size % 2048 == 0 and len(header) == 17*2048 and
                        header[:16] == b'SEGADISCSYSTEM  ' and header[32768:32775] == b'\x01CD001\x01',
                        'Expected a sector-aligned Megadev Sega CD ISO9660 data track')
            else:
                try:
                    with wave.open(str(dest), 'rb') as audio:
                        require((audio.getnchannels(), audio.getsampwidth(), audio.getframerate(), audio.getcomptype()) ==
                                (2, 2, 44100, 'NONE'), 'CD audio needs stereo 16-bit 44100 Hz PCM WAV')
                        frames = audio.getnframes()
                        require(frames % 588 == 0 and frames//588-track['index']['01'] >= 300,
                                'Audio needs whole sectors and four seconds after INDEX 01')
                        read = 0
                        while block := audio.readframes(65536):
                            read += len(block)
                        require(read == frames*4, 'Truncated WAV PCM')
                except (wave.Error, EOFError) as error:
                    raise ValueError('Invalid PCM WAV track: '+name) from error
            files[name] = dest; metadata[name] = {'bytes': size, 'sha256': digest.hexdigest()}
        (root / entry).write_bytes(cue)
        files[entry] = root / entry  # Publish CUE last, after every dependency.
        metadata[entry] = {'bytes': len(cue), 'sha256': hashlib.sha256(cue).hexdigest()}
        yield entry, files, metadata

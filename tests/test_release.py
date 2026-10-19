"""Small synthetic disc failures; actual SDK regional blocks run in target CI."""
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
import wave
import zipfile
import zlib
from test_project import ROOT,load

SCRIPT=ROOT/'skills/megadev-release/scripts/release.py'

def both(value,width):return value.to_bytes(width,'little')+value.to_bytes(width,'big')

def record(name,sector,size,directory=False):
    raw=bytearray(33+len(name)+(len(name)%2==0))
    raw[0]=len(raw);raw[2:10]=both(sector,4);raw[10:18]=both(size,4)
    raw[25]=2 if directory else 0;raw[28:32]=both(1,2);raw[32]=len(name);raw[33:33+len(name)]=name
    return raw

def fixture(root):
    root=Path(root);(root/'res').mkdir();(root/'disc').mkdir();(root/'build').mkdir()
    ip=b'SECURITY'+b'\0'*56;sp=b'SP'+b'\0'*126
    (root/'build/ip.bin').write_bytes(ip);(root/'build/sp.bin').write_bytes(sp)
    data=bytearray(32*2048);data[:16]=b'SEGADISCSYSTEM  '
    data[0x30:0x50]=struct.pack('>8I',0x800,0x800,0,0,0x1000,len(sp),0,0)
    data[0x200:0x200+len(ip)]=ip;data[0x1000:0x1000+len(sp)]=sp
    pvd=memoryview(data)[16*2048:17*2048];pvd[:7]=b'\1CD001\1';pvd[80:88]=both(32,4);pvd[120:124]=both(1,2);pvd[124:128]=both(1,2);pvd[128:132]=both(2048,2)
    rr=record(b'\0',20,2048,True);pvd[156:156+len(rr)]=rr
    data[17*2048:17*2048+7]=b'\xffCD001\1'
    entries=rr+record(b'\1',20,2048,True)+record(b'IPX.MMD;1',21,4)+record(b'SPX.SMD;1',22,4)
    data[20*2048:20*2048+len(entries)]=entries
    for name,sector,payload in [('ipx.mmd',21,b'MAIN'),('spx.smd',22,b'SUB!')]:
        data[sector*2048:sector*2048+4]=payload;(root/'disc'/name).write_bytes(payload)
    (root/'game.iso').write_bytes(data)
    pcm=bytes(44100*2*4)+struct.pack('<hh',2000,-2000)*(44100*4)
    for number in (2,3):
        with wave.open(str(root/'res'/f'track{number:02}.wav'),'wb') as w:
            w.setparams((2,2,44100,0,'NONE','not compressed'));w.writeframes(pcm)
    cue='FILE "game.iso" BINARY\n TRACK 01 MODE1/2048\n INDEX 01 00:00:00\n'
    for n in (2,3):cue+=f'FILE "res/track{n:02}.wav" WAVE\n TRACK {n:02} AUDIO\n INDEX 00 00:00:00\n INDEX 01 00:02:00\n'
    (root/'game.cue').write_text(cue);(root/'README.txt').write_text('Example controls; no BIOS included.\n')
    return root/'game.cue'


class ReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='release test ');self.root=Path(self.temp.name)
        self.addCleanup(self.temp.cleanup)
        self.api=load(SCRIPT,'release_under_test')
        self.api.SIGNATURES={**self.api.SIGNATURES,'US':(8,zlib.crc32(b'SECURITY'))}
        self.cue=fixture(self.root)
        self.provenance={'source_commit':'a'*40,'sdk_commit':'b'*40,'compiler':'m68k-elf-gcc test','container_reference':'ewrogers/megadev:latest'}
        self.contract=self.api.contract(self.cue,'US','NTSC',self.root/'disc',self.root/'build')

    def test_round_trip_reproducible_package_and_explicit_contents(self):
        (self.root/'private.bin').write_bytes(b'not selected')
        first=self.root/'one.zip';second=self.root/'two.zip'
        expected=self.api.package(self.cue,self.contract,self.provenance,first,['README.txt'])
        self.assertEqual(self.api.verify(first),expected)
        self.api.package(self.cue,self.contract,self.provenance,second,['README.txt'])
        self.assertEqual(first.read_bytes(),second.read_bytes())
        with zipfile.ZipFile(first) as z:
            self.assertNotIn('private.bin',z.namelist());self.assertNotIn('build/ip.bin',z.namelist())
        with self.assertRaises(FileExistsError):self.api.package(self.cue,self.contract,self.provenance,first)

    def test_contract_detects_compiled_payload_and_boot_mismatch(self):
        for file in ['disc/ipx.mmd','build/ip.bin','build/sp.bin']:
            p=self.root/file;old=p.read_bytes();p.write_bytes(b'WRONG')
            with self.assertRaises(ValueError):self.api.contract(self.cue,'US','NTSC',self.root/'disc',self.root/'build')
            p.write_bytes(old)

    def test_missing_audio_and_non_audio_configuration_fail(self):
        wav=self.root/'res/track02.wav';old=wav.read_bytes();wav.unlink()
        with self.assertRaises(ValueError):self.api.validate(self.cue,self.contract)
        with wave.open(str(wav),'wb') as w:w.setparams((1,1,22050,0,'NONE',''));w.writeframes(bytes(22050*6))
        with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')
        wav.write_bytes(old[:-17])
        with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')

    def test_audio_mutation_and_reordered_tracks_fail_approved_contract(self):
        p=self.root/'res/track02.wav';data=bytearray(p.read_bytes());data[-10]^=1;p.write_bytes(data)
        with self.assertRaises(ValueError):self.api.validate(self.cue,self.contract)
        text=self.cue.read_text().replace('track02.wav','other.wav').replace('track03.wav','track02.wav').replace('other.wav','track03.wav')
        self.cue.write_text(text)
        with self.assertRaises(ValueError):self.api.validate(self.cue,self.contract)

    def test_indices_pregaps_order_and_unsupported_formats_fail(self):
        original=self.cue.read_text()
        for before,after in [('TRACK 02','TRACK 04'),('00:02:00','00:02:75'),('00:02:00','00:05:00'),('MODE1/2048','MODE1/2352'),('INDEX 01 00:00:00','INDEX 00 00:00:00'),('FILE "res/track02.wav" WAVE','FILE "res/track02.wav" BINARY')]:
            with self.subTest(after=after):
                self.cue.write_text(original.replace(before,after))
                with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')
        self.cue.write_text(original+'PREGAP 00:02:00\n')
        with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')

    def test_path_traversal_aliases_and_reserved_names_rejected(self):
        for name in ['../x','/x','C:/x','a\\x','a//x','CON','a/NUL.wav','a.','a/../b']:
            with self.subTest(name=name),self.assertRaises(ValueError):self.api.portable(name)
        self.cue.write_text(self.cue.read_text().replace('res/track03.wav','res/TRACK02.WAV'))
        with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')

    def test_symlink_cannot_be_included(self):
        path=self.root/'extra.txt'
        try:path.symlink_to(self.root/'README.txt')
        except OSError:self.skipTest('Symlink permission unavailable')
        with self.assertRaises(ValueError):self.api.package(self.cue,self.contract,self.provenance,self.root/'bad.zip',['extra.txt'])

    def test_iso_corruption_is_rejected_before_contract(self):
        path=self.root/'game.iso';original=path.read_bytes()
        patches=[(0,b'NOTSEGA!'),(0x204,b'BROKEN'),(0x44,(0x7002).to_bytes(4,'big')),(32768+84,(31).to_bytes(4,'big')),(32768+128,b'\0\4'),(17*2048,b'BAD'),(20*2048+68+6,(0xffffffff).to_bytes(4,'big'))]
        for offset,new in patches:
            with self.subTest(offset=offset):
                data=bytearray(original);data[offset:offset+len(new)]=new;path.write_bytes(data)
                with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')
        path.write_bytes(original[:-1])
        with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')

    def test_iso_alias_overlap_name_and_extent_rejected(self):
        path=self.root/'game.iso';original=path.read_bytes();start=20*2048+68
        for offset,new in [(start+2,both(20,4)),(start+10,both(100000,4)),(start+32,b'\xff'),(start+25,b'\x80')]:
            with self.subTest(offset=offset):
                data=bytearray(original);data[offset:offset+len(new)]=new;path.write_bytes(data)
                with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')

    def test_unknown_region_video_and_missing_provenance_rejected(self):
        for region,video in [('JP','NTSC'),('US','AUTO')]:
            with self.assertRaises(ValueError):self.api.contract(self.cue,region,video,self.root/'disc',self.root/'build')
        p=dict(self.provenance);del p['sdk_commit']
        with self.assertRaises(ValueError):self.api.package(self.cue,self.contract,p,self.root/'bad.zip')

    def test_archive_damage_extra_members_and_hash_rewrites_rejected(self):
        path=self.root/'good.zip';self.api.package(self.cue,self.contract,self.provenance,path)
        with zipfile.ZipFile(path) as z:files={n:z.read(n) for n in z.namelist()}
        for kind in ['changed','extra','traversal','missing','rewritten_hash']:
            content=dict(files)
            if kind=='extra':content['UNLISTED.txt']=b'x'
            elif kind=='traversal':content['../outside']=b'x'
            elif kind=='missing':del content['res/track02.wav']
            else:
                data=bytearray(content['game.iso']);data[-1]^=1;content['game.iso']=bytes(data)
                if kind=='rewritten_hash':
                    m=json.loads(content['RELEASE.json']);m['files']['game.iso']=self.api.digest(data);content['RELEASE.json']=self.api.encoded(m)
            bad=self.root/(kind+'.zip')
            with zipfile.ZipFile(bad,'w') as z:
                for n,d in content.items():z.writestr(n,d)
            with self.subTest(kind=kind),self.assertRaises(ValueError):self.api.verify(bad)

    def test_partial_audio_sector_and_data_only_supported_profile(self):
        path=self.root/'res/track02.wav'
        with wave.open(str(path),'wb') as w:
            w.setparams((2,2,44100,0,'NONE',''));w.writeframes(bytes((44100*6-1)*4))
        with self.assertRaises(ValueError):self.api.inspect(self.cue,'US')
        self.cue.write_text('FILE "game.iso" BINARY\n TRACK 01 MODE1/2048\n INDEX 01 00:00:00\n')
        result=self.api.inspect(self.cue,'US')
        self.assertEqual(len(result['tracks']),1)
        self.assertEqual(set(result['iso']['files']),{'IPX.MMD','SPX.SMD'})

    def test_contract_cli_preserves_existing_approval(self):
        # Run CLI with the real regional signature check: a synthetic boot block
        # must fail without producing an approval file or replacing an old one.
        import subprocess,sys
        output=self.root/'approved.json';output.write_text('keep')
        r=subprocess.run([sys.executable,str(SCRIPT),'contract',str(self.cue),'--region','US','--video','NTSC','--payloads',str(self.root/'disc'),'--boot',str(self.root/'build'),'--output',str(output)],capture_output=True,text=True)
        self.assertNotEqual(r.returncode,0);self.assertEqual(output.read_text(),'keep')

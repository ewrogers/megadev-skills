import copy
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from PIL import Image
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-assets'
LAB=SKILL/'assets/asset-lab'
sys.path.insert(0,str(SKILL/'scripts'))
import asset_codec as codec
from prepare_image import prepare
from verify_prepared import verify
create=load(SKILL/'scripts/create_project.py','assets_creator').create
PALETTE=[codec.cram((codec.LEVELS[i%8],codec.LEVELS[(i//8)*3],0)) for i in range(16)]


def repaired(data):
    data=bytearray(data);struct.pack_into('>I',data,24,zlib.crc32(data[32:]));return bytes(data)


class AssetTests(unittest.TestCase):
    def test_native_dimensions_nibbles_and_cram_endian(self):
        p=[i%16 for i in range(64)]
        self.assertEqual(codec.tile_bytes(p)[:8],bytes.fromhex('0123456789ABCDEF'))
        self.assertEqual(codec.tile_pixels(codec.tile_bytes(p)),p)
        self.assertEqual(codec.cram((255,0,0)),0x000E)
        self.assertEqual(codec.cram((0,255,0)),0x00E0)
        self.assertEqual(codec.cram((0,0,255)),0x0E00)
        with self.assertRaises(ValueError):codec.cram((200,0,0))
        self.assertEqual(codec.rgb(codec.cram((200,0,0),True)),(182,0,0))
        with self.assertRaises(ValueError):codec.rgb(1)
        with self.assertRaises(ValueError):codec.pack([0]*63,7,9,PALETTE)

    def test_background_flip_deduplication_round_trips_exactly(self):
        tile=[((x//2+y*3)%15)+1 for y in range(8) for x in range(8)]
        source=[]
        variants=[tile,codec.flipped(tile,True),codec.flipped(tile,False,True),codec.flipped(tile,True,True)]
        for y in range(8):
            for p in variants:source.extend(p[y*8:y*8+8])
        data=codec.pack(source,32,8,PALETTE)
        self.assertEqual(codec.inspect(data)['tiles'],1)
        self.assertEqual(codec.unpack(data),(32,8,source))
        rng=random.Random(42)
        source=[rng.randrange(16) for _ in range(32*32)]
        self.assertEqual(codec.unpack(codec.pack(source,32,32,PALETTE)),(32,32,source))

    def test_sprite_sheet_frames_use_column_major_tiles_without_deduplication(self):
        source=[(x//8)+(y//8)*4 for y in range(16) for x in range(32)]
        data=codec.pack(source,32,16,PALETTE,(16,16))
        first_indices=[codec.tile_pixels(data[64+i*32:96+i*32])[0] for i in range(8)]
        self.assertEqual(first_indices,[0,4,1,5,2,6,3,7])
        self.assertEqual(codec.unpack(data,2),(32,16,source))
        for frame in [(7,8),(40,16),(24,16)]:
            with self.assertRaises(ValueError):codec.pack(source,32,16,PALETTE,frame)
        for size in [8,16,24,32]:
            p=[1]*(size*size)
            self.assertEqual(codec.unpack(codec.pack(p,size,size,PALETTE,(size,size)),1),(size,size,p))

    def test_crc_geometry_and_map_validation_reject_corruption(self):
        data=codec.pack([1]*64,8,8,PALETTE)
        bad=[]
        q=bytearray(data);q[-1]^=1;bad.append(bytes(q))
        q=bytearray(data);q[4:6]=b'\0\2';bad.append(bytes(q))
        q=bytearray(data);q[28:32]=b'\0\0\0\1';bad.append(bytes(q))
        q=bytearray(data);q[32]|=0x10;bad.append(repaired(q))
        q=bytearray(data);q[-2:]=b'\xE0\0';bad.append(repaired(q))
        q=bytearray(data);q[-2:]=b'\0\1';bad.append(repaired(q))
        bad.extend([data[:20],data[:-1],data+b'\0'])
        for b in bad:
            with self.subTest(data=b),self.assertRaises(ValueError):codec.inspect(b)

    def test_png_preparation_preserves_pixels_and_rejects_silent_edits(self):
        with tempfile.TemporaryDirectory() as folder:
            d=Path(folder);palette=['#000000','#FF0000','#B60000']+['#FFFFFF']*13
            pal=d/'palette.json';pal.write_text(json.dumps(palette))
            image=d/'sprite.png';out=d/'sprite.mda'
            src=Image.new('RGBA',(16,16),(255,0,0,255));src.putpixel((0,0),(0,0,0,0));src.save(image)
            report=prepare(image,pal,out,(16,16))
            self.assertEqual(report['source_size'],[16,16])
            self.assertEqual(list(Image.open(out.with_suffix('.used.png')).get_flattened_data()),list(src.get_flattened_data()))
            with self.assertRaises(ValueError):prepare(image,pal,out,(16,16))
            src.putpixel((1,1),(200,0,0,255));src.save(image)
            with self.assertRaises(ValueError):prepare(image,pal,out,(16,16),force=True)
            report=prepare(image,pal,out,(16,16),snap=True,force=True)
            self.assertEqual(report['snapped_source_colors'],1)
            self.assertEqual(Image.open(out.with_suffix('.used.png')).getpixel((1,1)),(182,0,0,255))
            src.putpixel((1,1),(0,255,0,255));src.save(image)
            with self.assertRaises(ValueError):prepare(image,pal,out,(16,16),snap=True,force=True)
            src.putpixel((1,1),(255,0,0,128));src.save(image)
            with self.assertRaises(ValueError):prepare(image,pal,out,(16,16),force=True)
            # No derived-output path may overwrite either input, even with --force.
            target=d/'sprite.used.png';shutil.copyfile(image,target);original=target.read_bytes()
            with self.assertRaises(ValueError):prepare(target,pal,out,(16,16),force=True)
            self.assertEqual(target.read_bytes(),original)

    def test_index_zero_requires_an_explicit_background_contract(self):
        with tempfile.TemporaryDirectory() as folder:
            d=Path(folder);Image.new('RGBA',(8,8),(0,0,0,255)).save(d/'black.png')
            (d/'palette.json').write_text(json.dumps(['#000000']+['#FFFFFF']*15))
            with self.assertRaises(ValueError):prepare(d/'black.png',d/'palette.json',d/'black.mda')
            r=prepare(d/'black.png',d/'palette.json',d/'black.mda',backdrop=True)
            self.assertEqual(r['index_zero_mode'],'scene_backdrop')
            with self.assertRaises(ValueError):prepare(d/'black.png',d/'palette.json',d/'other.mda',(8,8),backdrop=True)

    def test_prepared_art_checks_staleness_and_project_boundaries(self):
        with tempfile.TemporaryDirectory() as folder:
            d=create(Path(folder)/'game');reports=list((d/'res').glob('*.asset.json'))
            self.assertTrue(verify(d,reports))
            q=d/'res/garden.png';q.write_bytes(q.read_bytes()+b'changed')
            with self.assertRaises(ValueError):verify(d,reports)
            path=d/'res/actor.asset.json';m=json.loads(path.read_text());outside=Path(folder)/'outside';outside.write_bytes(b'x')
            m['inputs'][0]={'path':'../../outside','sha256':hashlib.sha256(b'x').hexdigest()};path.write_text(json.dumps(m))
            with self.assertRaises(ValueError):verify(d,[path])

    def test_pixellab_sources_reprepare_without_resizing_or_color_changes(self):
        with tempfile.TemporaryDirectory() as folder:
            d=create(Path(folder)/'game')
            for name in ['garden','cavern','actor','panel']:
                image=d/'res'/(name+'.png');palette=d/'res'/(name+'.palette.json');out=d/'regenerated'/(name+'.mda')
                frame=(16,24) if name=='actor' else None
                r=prepare(image,palette,out,frame,backdrop=name in ['garden','cavern'])
                self.assertEqual(out.read_bytes(),(d/'res'/(name+'.mda')).read_bytes())
                self.assertEqual(r['snapped_source_colors'],0)
                visible=lambda im:[p if p[3] else (0,0,0,0) for p in im.convert('RGBA').get_flattened_data()]
                with Image.open(image) as source, Image.open(out.with_suffix('.used.png')) as used:
                    self.assertEqual(r['source_size'],list(source.size))
                    self.assertEqual(visible(used),visible(source))
            self.assertTrue(verify(d,list((d/'regenerated').glob('*.asset.json'))))

    def test_c_decoder_with_valid_and_corrupt_files_at_both_optimization_levels(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No host C compiler')
        with tempfile.TemporaryDirectory() as folder:
            d=Path(folder)
            for mode in ['-O0','-O3']:
                binary=d/('decoder'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/asset.c'),str(ROOT/'tests/fixtures/asset.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                run=subprocess.run(args,capture_output=True,text=True);self.assertEqual(run.returncode,0,run.stderr)
                for name in ['garden','cavern','actor','panel']:
                    data=(LAB/'res'/(name+'.mda')).read_bytes();info=codec.inspect(data)
                    cases=[(data,1),(data[:40],0),(data[:-1],0)]
                    bad=bytearray(data);bad[-1]^=1;cases.append((bytes(bad),0))
                    if not info['kind']:
                        bad=bytearray(data);bad[-2:]=b'\xE0\0';cases.append((repaired(bad),0))
                    for content,expected in cases:
                        path=d/'fixture.mda';path.write_bytes(content)
                        run=subprocess.run([str(binary),str(path),str(expected),str(info['width_tiles']),str(info['height_tiles']),str(info['tiles']),str(info['entries'])],capture_output=True,text=True)
                        self.assertEqual(run.returncode,0,run.stderr)

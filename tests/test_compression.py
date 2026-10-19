import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-compression';LAB=SKILL/'assets/compression-lab'
create=load(SKILL/'scripts/create_project.py','compression_creator').create
tool=load(LAB/'tools/make_compressed.py','compression_author')

def vectors():
    rng=0x1967;data=bytearray()
    for i in range(8192):rng=(rng*1664525+1013904223)&0xffffffff;data.append(rng>>24)
    return [b'X',b'XY',b'XYZ',b'A'*300,bytes(range(256))*12,bytes(data[:600]),bytes(data)+bytes(data[:256]),b'abcd'*3000]

class CompressionTests(unittest.TestCase):
    def test_bounds_truncation_history_and_timer_wrap_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C17 compiler required')
            self.skipTest('C17 compiler unavailable')
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder);source=['#include "compress.h"','#include <assert.h>','int main(void){']
            for i,raw in enumerate(vectors()):
                packed=tool.encode(raw)
                source+=['{static unsigned char const d[]={'+','.join(map(str,packed))+'};',
                         f'assert(cp_kos_valid(d,sizeof(d),{len(raw)}));',
                         f'assert(!cp_kos_valid(d,sizeof(d),{len(raw)+1}));',
                         f'for(unsigned n=0;n<sizeof(d);n++)assert(!cp_kos_valid(d,n,{len(raw)}));','}']
            source+=['{unsigned char d[]={2,0,255,249,0,0,0};assert(!cp_kos_valid(d,sizeof(d),3));}',
                     'assert(!cp_kos_valid(0,5,1));assert(cp_elapsed(4090,5,0,60)==11);',
                     'assert(cp_elapsed(3000,904,4,60)==2000);assert(cp_elapsed(0,404,8,60)==4500);',
                     'assert(cp_elapsed(0,520,32,50)==21000);assert(cp_elapsed(0,1,5,60)==65535);',
                     'assert(cp_elapsed(0,0,33,50)==65535);assert(cp_elapsed(0,0,0,59)==65535);return 0;}']
            (p/'probe.c').write_text('\n'.join(source))
            for opt in ['-O0','-O3']:
                exe=p/('bounds'+opt+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/compress.c'),str(p/'probe.c'),'-o',str(exe)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                subprocess.run(args,check=True,capture_output=True);subprocess.run([str(exe)],check=True,capture_output=True)
    def test_sector_budgets_incompressible_sample_and_standalone_generator(self):
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'compression game')
            with self.assertRaises(ValueError):create(game)
            manifest=tool.generate(game);self.assertEqual(manifest,tool.generate(game))
            self.assertLess(manifest[1]['sectors'],manifest[0]['sectors'])
            self.assertGreater(manifest[5]['sectors'],manifest[4]['sectors'])
            for entry in manifest:
                self.assertEqual(entry['sectors'],(entry['bytes']+2047)//2048)
                self.assertEqual(entry['active_buffer_bytes'],entry['sectors']*2048+entry['output_bytes'])
                self.assertLessEqual(entry['sectors']*2048,32768)
            damaged=game/'disc/garden.kos';damaged.write_bytes(b'bad');tool.generate(game)
            self.assertEqual(len(damaged.read_bytes()),manifest[1]['bytes'])
            with self.assertRaises(ValueError):tool.encode(b'')
            with self.assertRaises(ValueError):tool.encode(bytes(32769))

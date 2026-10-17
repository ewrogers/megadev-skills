import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_project import ROOT,load
SKILL=ROOT/'skills/megadev-graphics'
LAB=SKILL/'assets/graphics-lab'
create=load(SKILL/'scripts/create_project.py','graphics_creator').create
class GraphicsTests(unittest.TestCase):
    def test_renderer_contracts_at_O0_O3(self):
        cc=next((p for n in ['clang','cc','gcc'] if (p:=shutil.which(n))),None)
        if not cc:
            if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':self.fail('C compiler required')
            self.skipTest('No C compiler')
        with tempfile.TemporaryDirectory() as folder:
            for mode in ['-O0','-O3']:
                binary=Path(folder)/('gfx'+mode+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',mode,'-Wall','-Wextra','-Werror','-I',str(LAB/'src'),str(LAB/'src/gfx.c'),str(ROOT/'tests/fixtures/gfx.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
    def test_standalone_generation_and_exact_art_round_trip(self):
        import sys
        sys.path.insert(0,str(ROOT/'skills/megadev-assets/scripts'))
        from prepare_image import prepare
        with tempfile.TemporaryDirectory() as folder:
            game=create(Path(folder)/'graphics game')
            self.assertTrue((game/'src/gfx.c').is_file())
            with self.assertRaises(ValueError):create(game)
            for name in ['sky','quay','ship']:
                source=game/'res';output=game/'regenerated'/(name+'.mda')
                report=prepare(source/(name+'.png'),source/(name+'.palette.json'),output,(32,24) if name=='ship' else None,backdrop=name=='sky')
                self.assertEqual(report['snapped_source_colors'],0)
                self.assertEqual(output.read_bytes(),(source/(name+'.mda')).read_bytes())
    def test_pixel_sources_repeat_without_seams_and_font_is_reproducible(self):
        from PIL import Image
        for name in ['sky','quay']:
            with Image.open(LAB/'res'/(name+'.png')) as opened:
                image=opened.convert('RGBA');self.assertEqual(image.size,(512,192))
                self.assertEqual(list(image.crop((0,0,1,192)).get_flattened_data()),list(image.crop((511,0,512,192)).get_flattened_data()))
                self.assertEqual(list(image.crop((255,0,256,192)).get_flattened_data()),list(image.crop((256,0,257,192)).get_flattened_data()))
        font=load(LAB/'tools/make_font.py','graphics_font')
        self.assertEqual(font.build(),(LAB/'res/font.bin').read_bytes())
        self.assertEqual(len(font.build()),96*32)
        self.assertTrue(all((b>>4) in [1,2] and (b&15) in [1,2] for b in font.build()))

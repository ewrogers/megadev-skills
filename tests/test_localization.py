"""UTF-8 authoring, native glyphs, target-independent layout and safe UI state."""
import copy,ctypes as c,json,os,shutil,struct,subprocess,sys,tempfile,unicodedata,unittest
from pathlib import Path
from test_project import ROOT,load
LAB=ROOT/'skills/megadev-localization/assets/language-lab'
sys.path.insert(0,str(LAB/'tools'))
compiler=load(LAB/'tools/catalog.py','language_catalog')
sys.path.pop(0)
class Model(c.Structure):_fields_=[(n,c.c_ushort) for n in ['page','language','focus','cursor','name_length','quantity','dialog_page','paused','clock','repeat_key','repeat_age','visited']]+[('name',c.c_ubyte*8)]
class Canvas(c.Structure):_fields_=[('cells',c.c_ushort*1120)]
class Layout(c.Structure):_fields_=[('cells',c.c_ubyte*1360),('pages',c.c_ushort),('width',c.c_ushort),('lines',c.c_ushort)]
class LanguageTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup);cls.folder=Path(cls.temp.name)
  cls.cc=shutil.which('clang') or shutil.which('cc') or shutil.which('gcc')
  if not cls.cc:
   if os.environ.get('MEGADEV_REQUIRE_HOST_CC')=='1':raise RuntimeError('C17 compiler required')
   raise unittest.SkipTest('C17 compiler required')
  dll=cls.folder/('language.dll' if os.name=='nt' else 'language.so')
  subprocess.run([cls.cc,'-shared','-DLC_HOST_DLL','-O3',*(['-fPIC'] if os.name!='nt' else []),*[str(LAB/'src'/n) for n in ['loc.c','catalog.c','ui.c','paint.c']],'-o',str(dll)],check=True,capture_output=True)
  cls.lib=c.CDLL(str(dll))
  if os.name=='nt':
   import _ctypes
   cls.addClassCleanup(_ctypes.FreeLibrary,cls.lib._handle)
  cls.lib.ui_init.argtypes=[c.POINTER(Model)];cls.lib.ui_input.argtypes=[c.POINTER(Model),c.c_ushort,c.c_ushort,c.c_ushort]
  cls.lib.ui_draw.argtypes=[c.POINTER(Canvas),c.POINTER(Model),c.c_ushort]
  cls.lib.lc_format.argtypes=[c.c_ushort,c.c_ushort,c.POINTER(c.c_ubyte),c.c_ushort,c.c_ushort,c.POINTER(c.c_ubyte),c.c_ushort];cls.lib.lc_format.restype=c.c_ushort
  cls.lib.lc_wrap.argtypes=[c.POINTER(c.c_ubyte),c.c_ushort,c.c_ushort,c.c_ushort,c.POINTER(Layout)];cls.lib.lc_wrap.restype=c.c_ushort
  cls.data=compiler.compile_data(LAB/'res/catalog.json');cls.source=json.loads((LAB/'res/catalog.json').read_text(encoding="utf-8"))
 def model(self):
  m=Model();self.lib.ui_init(c.byref(m));return m
 def format(self,lang,mid,name=None,quantity=65535):
  out=(c.c_ubyte*384)();m=self.model();n=self.lib.lc_format(lang,self.data['ids'].index(mid),m.name if name is None else name,m.name_length if name is None else len(name),quantity,out,384)
  self.assertTrue(n);return bytes(out)[:n-1]
 def decode(self,b):return ''.join('\n' if n==252 else self.data['glyphs'][n-1] for n in b)
 def test_catalog_normalization_explicit_ids_and_regeneration(self):
  d=copy.deepcopy(self.source);d['messages']['language'][1]=unicodedata.normalize('NFD',d['messages']['language'][1]);d['messages']=dict(reversed(list(d['messages'].items())))
  actual=compiler.compile_data(d);self.assertEqual(actual['blob'],self.data['blob']);self.assertEqual(actual['glyphs'],self.data['glyphs'])
  report=compiler.generate(LAB/'res/catalog.json',self.folder/'generated')
  self.assertEqual(report['pattern_bytes'],report['glyphs']*32);self.assertLessEqual(report['glyphs'],192)
  for n in ['catalog.c','catalog.h']:self.assertEqual((self.folder/'generated/src'/n).read_bytes(),(LAB/'src'/n).read_bytes())
 def test_catalog_rejects_missing_translation_tokens_controls_and_glyphs(self):
  changes=[lambda d:d['messages']['talk'].__setitem__(0,''),lambda d:d['messages']['talk'].__setitem__(0,'   '),lambda d:d['messages']['story'].__setitem__(1,'Missing name'),lambda d:d['messages']['story'].__setitem__(0,'{unknown}'),lambda d:d['messages']['story'].__setitem__(0,'{{name}'),lambda d:d['messages']['talk'].pop(),lambda d:d['messages']['talk'].__setitem__(0,'Bad\ttext'),lambda d:d['messages']['talk'].__setitem__(0,'Missing ☃'),lambda d:d['messages']['talk'].__setitem__(0,'W'*40),lambda d:d['ids'].append('talk')]
  for change in changes:
   d=copy.deepcopy(self.source);change(d)
   with self.assertRaises((ValueError,KeyError,TypeError)):compiler.compile_data(d)
  with self.assertRaises(ValueError):json.loads('{"a":1,"a":2}',object_pairs_hook=compiler.unique)
  d=copy.deepcopy(self.source);d['messages']['story'][0]=''.join(chr(0x400+i) for i in range(100))+' {name}'
  with self.assertRaisesRegex(ValueError,'reservation'):compiler.compile_data(d)
 def test_format_matches_readable_catalog_and_guards(self):
  for lang in range(4):
   for mid in self.data['ids']:
    expected=self.data['normalized'][mid][lang].replace('{name}','LÉO').replace('{count}','65535')
    self.assertEqual(self.decode(self.format(lang,mid)),expected)
  self.assertEqual(self.format(65535,'title'),self.format(0,'title'))
  m=self.model();out=(c.c_ubyte*20)(*([0xA5]*20));before=bytes(out)
  self.assertFalse(self.lib.lc_format(0,12,m.name,3,0,out,2));self.assertEqual(bytes(out),before)
  bad=(c.c_ubyte*1)(253);self.assertFalse(self.lib.lc_format(0,12,bad,1,0,out,20));self.assertEqual(bytes(out),before)
 def test_wrap_word_boundaries_pages_and_failure_atomicity(self):
  codec=self.data['codec'];s='AB CDE FGHIJKLMNOP\nQR ST UV'
  for width in [1,4,8,24,34]:
   for lines in [1,2,5]:
    words=[]
    for paragraph in s.split('\n'):
     line=''
     for word in paragraph.split():
      if line and len(line)+1+len(word)>width:words.append(line);line=''
      while len(word)>width:words.append(word[:width]);word=word[width:]
      if word:line=(line+' ' if line else '')+word
     words.append(line)
    expected=[words[i:i+lines] for i in range(0,len(words),lines)]
    encoded=(c.c_ubyte*len(s))(*[252 if ch=='\n' else codec[ch] for ch in s]);l=Layout();c.memset(c.byref(l),0xA5,c.sizeof(l));before=bytes(l)
    n=self.lib.lc_wrap(encoded,len(s),width,lines,c.byref(l))
    if len(expected)>8:self.assertFalse(n);self.assertEqual(bytes(l),before);continue
    self.assertEqual(n,len(expected),(width,lines))
    for p,page in enumerate(expected):
     for y,row in enumerate(page):self.assertEqual(self.decode(bytes(l.cells[(p*5+y)*34:(p*5+y)*34+width])).rstrip(),row)
 def test_every_translated_layout_and_font_mapping(self):
  for lang in range(4):
   for page in range(5):
    for paused in [0,1]:
     m=self.model();m.language=lang;m.page=page;m.paused=paused;m.name_length=8;m.quantity=65535
     for i in range(8):m.name[i]=self.data['codec']['W']
     canvas=Canvas();self.assertTrue(self.lib.ui_draw(c.byref(canvas),c.byref(m),60),(lang,page,paused));self.assertTrue(all((v&2047)<0x138 for v in canvas.cells))
     if not paused:self.assertTrue(all((canvas.cells[22*40+x]&2047)==0xE7 for x in range(2,38)),'Text overwrote bottom panel border')
  m=self.model();canvas=Canvas();self.assertTrue(self.lib.ui_draw(c.byref(canvas),c.byref(m),60))
  self.assertTrue(all(((canvas.cells[11*40+x]>>13)&3)==1 for x in range(5,22)),'Empty second menu line was highlighted')
  m.language=1;m.focus=3;self.assertTrue(self.lib.ui_draw(c.byref(canvas),c.byref(m),60))
  self.assertTrue(any(((canvas.cells[20*40+x]>>13)&3)==2 for x in range(5,22)),'Wrapped French label lost its real second line')
  m=self.model();m.language=4;canvas=Canvas();before=bytes(canvas);self.assertFalse(self.lib.ui_draw(c.byref(canvas),c.byref(m),60));self.assertEqual(bytes(canvas),before)
  fonts=(LAB/'res/fonts.bin').read_bytes();self.assertEqual(len(fonts),192*32)
  for index,ch in enumerate(self.data['glyphs']):
   pattern=fonts[index*32:index*32+32];rows=compiler.rows(ch)
   for y,bits in enumerate(rows):
    pixels=[(pattern[y*4+x//2]>>(0 if x&1 else 4))&15 for x in range(8)]
    self.assertEqual(pixels,[2 if bits&(128>>x) else 1 for x in range(8)],ch)
 def test_navigation_repeat_pause_name_bounds_and_focus(self):
  a=self.model();b=self.model()
  for m in [a,b]:self.lib.ui_input(c.byref(m),2,2,0)
  for _ in range(60):self.lib.ui_input(c.byref(a),0,2,5)
  for _ in range(50):self.lib.ui_input(c.byref(b),0,2,6)
  self.assertEqual(bytes(a),bytes(b))
  self.lib.ui_input(c.byref(a),128,0,0);before=bytes(a);self.lib.ui_input(c.byref(a),112,15,30);self.assertEqual(bytes(a),before)
  m=self.model();m.focus=2;self.lib.ui_input(c.byref(m),64,0,0)
  for _ in range(30):self.lib.ui_input(c.byref(m),64,0,0)
  self.assertEqual(m.name_length,8);m.cursor=43;self.lib.ui_input(c.byref(m),64,0,0);self.assertEqual((m.page,m.focus),(0,2))
  m.page=1;m.dialog_page=2;self.lib.ui_input(c.byref(m),32,0,0);self.assertEqual((m.language,m.dialog_page),(1,0))
 def test_standalone_creation_and_optimization_contracts(self):
  create=load(ROOT/'skills/megadev-localization/scripts/create_project.py','language_creator').create
  with tempfile.TemporaryDirectory() as d:
   p=create(Path(d)/'language game');self.assertTrue((p/'src/loc.c').is_file())
   with self.assertRaises(ValueError):create(p)
   for bad in ['../escape','$(id)','Upper']:
    with self.assertRaises(ValueError):create(Path(d)/'bad',project_id=bad)
  r=subprocess.run([sys.executable,str(LAB/'tools/check_language.py')],capture_output=True,text=True,check=True);self.assertTrue(json.loads(r.stdout)['passed'])

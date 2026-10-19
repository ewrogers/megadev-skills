"""Execute original-68000 catalog/layout/UI and immutable VBlank publication.
MMIO flags/scanlines are modeled; this is not video or bus timing evidence.
"""
import json,struct,unicodedata
from pathlib import Path
LAB=Path(__file__).resolve().parents[1]/'skills/megadev-localization/assets/language-lab'
def check_localization(u,sym):
 from unicorn import UC_HOOK_MEM_WRITE
 import unicorn.m68k_const as r
 stop=0x300000;stack=0xFFDF00;model=0x210000;canvas=model+256;painter=canvas+2240;batch=painter+4496;text=batch+512;layout=text+512
 u.mem_map(stop,4096);u.mem_map(0xC00000,4096)
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,v):u.mem_write(a,struct.pack('>H',v&65535))
 def call(name,args=(),limit=3000000):
  u.mem_write(0xFFCFE0,b'\xA5'*32)
  u.reg_write(r.UC_M68K_REG_A7,stack);u.mem_write(stack,struct.pack('>I',stop)+b''.join(struct.pack('>I',v&0xffffffff) for v in args))
  u.emu_start(sym[name],stop,count=limit)
  assert u.reg_read(r.UC_M68K_REG_PC)==stop,name+' did not finish on base 68000'
  assert u.reg_read(r.UC_M68K_REG_A7)==stack+4,name+' leaked C stack'
  assert bytes(u.mem_read(0xFFCFE0,32))==b'\xA5'*32,name+' exceeded reserved Main stack'
  return u.reg_read(r.UC_M68K_REG_D0)&65535
 source=json.loads((LAB/'res/catalog.json').read_text(encoding="utf-8"));report=json.loads((LAB/'res/catalog-report.json').read_text(encoding="utf-8"));glyphs=report['glyph_order'];codec={c:i+1 for i,c in enumerate(glyphs)}
 call('ui_init',(model,));assert word(model+8)==3
 for language in range(4):
  for message,mid in enumerate(source['ids']):
   n=call('lc_format',(language,message,model+24,3,65535,text,384));assert n
   expected=unicodedata.normalize('NFC',source['messages'][mid][language]).replace('{name}','LÉO').replace('{count}','65535')
   encoded=bytes(252 if c=='\n' else codec[c] for c in expected)+b'\0'
   assert bytes(u.mem_read(text,n))==encoded,(language,mid)
   assert call('lc_wrap',(text,n-1,24,5,layout))
 assert call('lc_language',(65535,))==0
 u.mem_write(text,b'\xA5'*384);assert not call('lc_format',(0,12,model+24,3,0,text,2));assert bytes(u.mem_read(text,384))==b'\xA5'*384
 u.mem_write(layout,b'\xA5'*1366);assert not call('lc_wrap',(text,1,0,5,layout));assert bytes(u.mem_read(layout,1366))==b'\xA5'*1366
 for language in range(4):
  for page in range(5):
   for paused in [0,1]:
    call('ui_init',(model,));put(model+2,language);put(model,page);put(model+14,paused);put(model+8,8);put(model+10,65535);u.mem_write(model+24,bytes([codec['W']])*8)
    assert call('ui_draw',(canvas,model,50)),(language,page,paused)
    assert all(word(canvas+i*2)&2047<0x138 for i in range(1120))
 call('ui_init',(model,));call('ui_input',(model,2,2,0))
 for _ in range(60):call('ui_input',(model,0,2,5))
 ntsc=bytes(u.mem_read(model,32));call('ui_init',(model,));call('ui_input',(model,2,2,0))
 for _ in range(50):call('ui_input',(model,0,2,6))
 assert bytes(u.mem_read(model,32))==ntsc
 call('ui_input',(model,128,0,0));paused=bytes(u.mem_read(model,32));call('ui_input',(model,112,15,30));assert bytes(u.mem_read(model,32))==paused
 call('ui_init',(model,));put(model+4,2);call('ui_input',(model,64,0,0))
 for _ in range(20):call('ui_input',(model,64,0,0))
 assert word(model+8)==8;put(model+6,43);call('ui_input',(model,64,0,0));assert word(model)==0 and word(model+4)==2
 call('ui_init',(model,));call('ui_draw',(canvas,model,60));call('ui_painter_init',(painter,))
 writes=[]
 def capture(cpu,access,address,size,value,user):
  if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hook=u.hook_add(UC_HOOK_MEM_WRITE,capture);u.ctl_flush_tb()
 def irq(status=8,line=224):
  put(0xC00004,status);put(0xC00008,line<<8);writes.clear();call('vblank_user')
 for step in range(7):
  assert call('ui_prepare',(painter,canvas,0,batch))
  expected=bytes(u.mem_read(batch+16,320));u.mem_write(sym['batch'],bytes(u.mem_read(batch,336)));put(sym['published'],1)
  if step==0:
   for status,line in [(0,224),(8,223),(8,231)]:irq(status,line);assert not writes and word(sym['published'])==1
   immutable=bytes(u.mem_read(batch,336));assert not call('ui_prepare',(painter,canvas,16,batch));assert bytes(u.mem_read(batch,336))==immutable
  irq();assert not word(sym['published']) and word(sym['consumed'])==word(batch+6)
  assert b''.join(v.to_bytes(size,'big') for a,size,v in writes if a==0xC00000)==expected
  controls=[v for a,size,v in writes if a==0xC00004 and size==2];assert controls==([0x8238] if step==6 else []),'Incomplete translated page became visible'
  assert call('ui_ack',(painter,batch))
 assert word(0xFFFDB8)==0x8238
 irq();assert not writes,'Idle page kept redrawing'
 put(sym['batch'],5);put(sym['published'],1);irq();assert not writes and word(sym['video_fault'])==1
 if 'ui_tour' in sym:
  gm,gc,gp,gb=(sym[n] for n in ['model','desired','painter','batch'])
  put(sym['published'],0);put(sym['video_fault'],0);call('ui_init',(gm,));call('ui_painter_init',(gp,));visited=0;languages=0
  for _ in range(260):
   call('ui_tour',(150,));assert call('ui_draw',(gc,gm,60))
   key=call('ui_key',(gm,))
   if call('ui_prepare',(gp,gc,key,gb)):
    put(sym['published'],1);irq();assert not word(sym['published']);assert call('ui_ack',(gp,gb))
    if not word(gp+4488):visited|=1<<word(gm);languages|=1<<word(gm+2)
  assert visited==31 and languages==15,(visited,languages)
 u.hook_del(hook);return True

def check_rebuild(project,run):
 """Changing a UTF-8 catalog must update both executable text and font patterns."""
 from startup import check,symbols
 import unicorn.m68k_const as r
 catalog=project/'res/catalog.json';original=catalog.read_bytes();baseline=(project/'disc/ipx.mmd').read_bytes();font=(project/'res/fonts.bin').read_bytes()
 try:
  data=json.loads(original);data['messages']['title'][0]+=' à'
  catalog.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')
  run('language-incremental-mutated',['build','-C',project.name,'REGION=US','VIDEO=NTSC'])
  payload=(project/'disc/ipx.mmd').read_bytes();sy=symbols((project/'build/ipx.mmd.elf.sym').read_text())
  report=json.loads((project/'res/catalog-report.json').read_text(encoding='utf-8'));codec={c:i+1 for i,c in enumerate(report['glyph_order'])}
  expected=bytes(codec[c] for c in data['messages']['title'][0])+b'\0';expected_font=(project/'res/fonts.bin').read_bytes()
  assert payload!=baseline and expected_font!=font,'Catalog edit failed to regenerate font/payload'
  def execute(u,sym):
   stop=0x300000;stack=0xFFDF00;out=0x210000;u.mem_map(stop,4096)
   u.reg_write(r.UC_M68K_REG_A7,stack);u.mem_write(stack,struct.pack('>8I',stop,0,1,0,0,0,out,384))
   u.emu_start(sym['lc_format'],stop,count=1000000);assert u.reg_read(r.UC_M68K_REG_PC)==stop
   assert bytes(u.mem_read(out,len(expected)))==expected,'Stale compiled translation after edit'
   assert bytes(u.mem_read(sym['res_fonts'],len(expected_font)))==expected_font,'Stale incbin font after edit'
   return True
  check(payload,sy,'main',execute)
 finally:
  catalog.write_bytes(original)
  run('language-incremental-restored',['build','-C',project.name,'REGION=US','VIDEO=NTSC'])
 assert (project/'disc/ipx.mmd').read_bytes()==baseline,'Restored catalog did not reproduce baseline'
 return True

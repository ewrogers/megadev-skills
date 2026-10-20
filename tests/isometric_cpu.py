"""Original-M68000 geometry, packet decode and immutable VDP publication.
Registers are modeled; this does not qualify pixels, sound or physical timing.
"""
def check_isometric(u,sym,res):
 from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7 as SP,UC_M68K_REG_PC as PC,UC_M68K_REG_D0 as D0,UC_M68K_REG_SR as SR
 stop=0x300000;stack=0xffdf00
 u.mem_map(stop,0x1000);u.mem_map(0xc00000,0x1000)
 def call(name,args=(),count=60000000):
  u.reg_write(SR,0x2700);u.reg_write(SP,stack)
  u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
  u.emu_start(sym[name],stop,count=count)
  assert u.reg_read(PC)==stop,(name,'did not return',hex(u.reg_read(PC)))
  assert u.reg_read(SP)==stack+4,name+' changed stack'
  return u.reg_read(D0)&65535
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,n):u.mem_write(a,(n&65535).to_bytes(2,'big'))
 result=call('isometric_contract');assert result==0,('Target isometric contract failed',result)
 assert sym['_BSS_ORIGIN']+sym['_BSS_LENGTH']<=0xffd000
 # Execute the ABI adapter: Word RAM must use BIOS 0x2D4, not ordinary DMA.
 from unicorn.m68k_const import UC_M68K_REG_D1 as D1,UC_M68K_REG_D2 as D2,UC_M68K_REG_D3 as D3,UC_M68K_REG_A6 as A6
 u.mem_write(0x2d4,b'\x4e\x75');seen=[]
 for reg in [D2,D3,A6]:u.reg_write(reg,0x12345678)
 def bios(cpu,address,size,user):
  if address==0x2d4:
   seen.append((cpu.reg_read(D0),cpu.reg_read(D1),cpu.reg_read(D2)))
   for reg in [D2,D3,A6]:cpu.reg_write(reg,0xdeadbeef)
 hook=u.hook_add(UC_HOOK_CODE,bios,begin=0x2d4,end=0x2d4)
 call('lab_upload_word',(0x200000,0x40000000,16));u.hook_del(hook)
 assert seen==[(0x40000000,0x200000,16)]
 assert all(u.reg_read(reg)==0x12345678 for reg in [D2,D3,A6])
 for name in ['actors','ground','map']:
  raw=(res/(name+'.bin')).read_bytes();start=sym['res_'+name];size=sym['res_'+name+'_end']-start
  assert call('iso_unpack',(start,size,0x200000,len(raw)))==1
  assert bytes(u.mem_read(0x200000,len(raw)))==raw
  assert call('iso_unpack',(start,size-1,0x200000,len(raw)))==0
 # Execute the real initializer with modeled DMA/VDP memory, not just the decoder.
 # The wrapper ABI was checked above; here ordinary-vs-Word-RAM selection matters.
 vram=bytearray(65536);cram=bytearray(128);port=[0,0];dmas=[]
 def address(command):return ((command>>16)&0x3fff)|((command&3)<<14)
 def init_code(cpu,pc,size,user):
  names={sym[n]:n for n in ['lab_display_off','lab_clear','lab_upload_work','lab_upload_word']}
  if pc not in names:return
  name=names[pc];sp=cpu.reg_read(SP)
  if name.startswith('lab_upload_'):
   source=int.from_bytes(cpu.mem_read(sp+4,4),'big');dest=int.from_bytes(cpu.mem_read(sp+8,4),'big');words=int.from_bytes(cpu.mem_read(sp+12,4),'big')
   if name=='lab_upload_word':assert 0x200000<=source<0x240000
   else:assert source>=0xff0000,'Word RAM art used the ordinary DMA path'
   at=address(dest);vram[at:at+words*2]=bytes(cpu.mem_read(source,words*2));dmas.append(name)
  ret=int.from_bytes(cpu.mem_read(sp,4),'big');cpu.reg_write(SP,sp+4);cpu.reg_write(PC,ret)
 def init_write(cpu,access,at,size,value,user):
  if at==0xc00004 and size==4:port[:]=[1 if value&0xc0000000==0xc0000000 else 2 if value&0x10 else 0,address(value)]
  if at==0xc00000:
   target=cram if port[0]==1 else vram if port[0]==0 else None
   if target is not None:target[port[1]:port[1]+size]=(value&((1<<(size*8))-1)).to_bytes(size,'big')
   port[1]+=size
 ih=u.hook_add(UC_HOOK_CODE,init_code);iw=u.hook_add(UC_HOOK_MEM_WRITE,init_write)
 call('isometric_begin_probe');call('isometric_initialize');u.hook_del(ih);u.hook_del(iw)
 assert dmas==['lab_upload_work','lab_upload_word','lab_upload_word']
 assert vram[0x1000:0x1000+11776]==(res/'actors.bin').read_bytes()
 assert vram[0x5000:0x5000+8704]==(res/'ground.bin').read_bytes()
 assert cram==(res/'palettes.bin').read_bytes()
 original=(res/'map.bin').read_bytes()
 expected_map=b''.join((0x2280+int.from_bytes(original[i:i+2],'big')).to_bytes(2,'big') for i in range(0,4096,2))
 assert vram[0xe000:0xf000]==expected_map
 call('isometric_begin_probe');call('isometric_publish')
 assert word(sym['published'])==1
 frame=sym['frames'];count=word(frame+1312);assert 10<=count<=56
 snapshot=bytes(u.mem_read(frame,1644));call('isometric_publish')
 assert bytes(u.mem_read(frame,1644))==snapshot
 writes=[];early=[False]
 def read(cpu,access,address,size,value,user):
  if address==0xc00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
  if address==0xc00008:cpu.mem_write(address,(0xe000 if early[0] else 0xe900).to_bytes(size,'big'))
 def write(cpu,access,address,size,value,user):
  if 0xc00000<=address<=0xc00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 call('vblank_user');assert not writes and word(sym['published'])==1
 early[0]=True;call('vblank_user');assert not word(sym['published'])
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xc00000)
 expected=bytes(u.mem_read(frame,count*8))+bytes(u.mem_read(frame+1320,320))+b'\0\0'+bytes(u.mem_read(frame+1640,2))+b'\0\0'+bytes(u.mem_read(frame+1642,2))
 assert actual==expected
 controls=[v for a,n,v in writes if a==0xc00004]
 assert len(controls)==7 and 0xc0000000 not in controls
 for stop_id in range(4):
  call('isometric_tour_probe',(stop_id,));call('isometric_publish')
  selected=frame+(word(sym['published'])-1)*1644
  assert not word(selected+1318),'Scanline admission dropped authored scenery'
  tiles=[word(selected+i*8+4)&0x7ff for i in range(word(selected+1312))]
  assert sum(302<=v<494 for v in tiles)==1,'Hero missing or duplicated'
  call('vblank_user')
 call('isometric_info_probe');call('isometric_publish')
 selected=frame+(word(sym['published'])-1)*1644
 assert word(selected+4)&0x7ff==494,'Diagnostic foot marker must be in front'
 assert not word(selected+1318)
 call('vblank_user')
 call('isometric_cutaway_probe');call('isometric_publish');selected=frame+(word(sym['published'])-1)*1644
 assert word(selected+1312)<count
 call('vblank_user');assert word(sym['upload_high'])<=776
 writes.clear();put(frame+1312,57);put(sym['published'],1);call('vblank_user')
 assert not writes and word(sym['video_fault']) and not word(sym['published'])
 put(sym['published'],3);call('vblank_user');assert not writes
 u.hook_del(hr);u.hook_del(hw)
 return True

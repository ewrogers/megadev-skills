"""Execute Main model/publication; IRQ/VDP capture does not measure raster time."""
def check_gameplay(u,sym):
 from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
 stop=0x300000;stack=0xFFDF00;model=0x210000
 u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
 def call(name,args=()):
  u.reg_write(UC_M68K_REG_A7,stack)
  u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((a&0xffffffff).to_bytes(4,'big') for a in args))
  u.emu_start(sym[name],stop,count=1000000)
  assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not finish'
  assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
  return u.reg_read(UC_M68K_REG_D0)&65535
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,n):u.mem_write(a,(n&65535).to_bytes(2,'big'))
 call('gp_init',(model,))
 for i in range(60):call('gp_advance',(model,5,8,0))
 expected=bytes(u.mem_read(model,172))
 call('gp_init',(model,))
 for i in range(50):call('gp_advance',(model,6,8,0))
 assert bytes(u.mem_read(model,172))==expected,'Target PAL/NTSC elapsed-second mismatch'
 call('gp_init',(model,));assert call('gp_advance',(model,65535,8,64))==4
 assert word(model+150)==4 and word(model+164)==1 and word(model+142)==177
 call('gp_advance',(model,5,0,128));paused=bytes(u.mem_read(model,172))
 for i in range(50):call('gp_advance',(model,6,8,64))
 assert bytes(u.mem_read(model,172))==paused,'Paused model changed'
 call('gp_init',(model,));put(model,424);put(model+2,320)
 for i in range(30):call('gp_move',(model,8,0))
 assert word(model)==428
 call('gp_move',(model,8,8));assert word(model)==428 and word(model+2)==328
 call('gp_init',(model,));put(model,1996);put(model+2,512)
 call('gp_advance',(model,5,8,0));assert word(model+132)==1 and word(model)==64 and word(model+168)==1
 for i in range(12):assert call('gp_spawn',(model,64,512,1))==1
 assert call('gp_spawn',(model,64,512,1))==0 and word(model+166)==1
 for i in range(24):call('gp_advance',(model,5,0,0))
 assert call('gp_spawn',(model,64,512,2))==1
 # Exercise real frame preparation; game data remains in Work RAM.
 call('gp_init',(sym['game'],));call('gameplay_publish')
 assert word(sym['published'])==1
 frame=sym['frames'];count=word(frame+1312);assert 1<=count<=28
 snapshot=bytes(u.mem_read(frame,1644));call('gameplay_publish')
 assert bytes(u.mem_read(frame,1644))==snapshot,'Overwrote published immutable frame'
 writes=[];early=True
 def read(cpu,access,address,size,value,user):
  if address==0xC00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
  if address==0xC00008:cpu.mem_write(address,(0xE000 if early else 0xE900).to_bytes(size,'big'))
 def write(cpu,access,address,size,value,user):
  if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 early=False;call('vblank_user');assert not writes and word(sym['published'])==1
 early=True;call('vblank_user');assert not word(sym['published']) and word(sym['submitted'])==1
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 # Spawn y=128 gives camera y=32, so playfield vertical scroll is zero.
 expected=bytes(u.mem_read(frame,count*8))+bytes(8)+bytes(u.mem_read(frame+1320,320))
 assert actual==expected,'Published SAT/scroll/HUD words differ'
 assert word(sym['upload_high'])==len(expected)<=552
 # The normal frame publisher never writes CRAM; four HUD row addresses stay fixed.
 controls=[v for a,n,v in writes if a==0xC00004]
 assert len(controls)==7 and 0xC0000000 not in controls
 writes.clear();call('vblank_user');assert not writes
 put(frame+1312,29);put(sym['published'],1);call('vblank_user')
 assert not writes and word(sym['video_fault'])==1 and not word(sym['published'])
 u.hook_del(hr);u.hook_del(hw)
 return True

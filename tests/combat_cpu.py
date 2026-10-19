"""Execute combat behavior and immutable VDP publication on original M68000.

VDP/interrupt registers are modeled. This does not measure console cycles,
drawn pixels, sound quality or physical controller behavior.
"""
def check_combat(u,sym):
 from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7 as SP,UC_M68K_REG_PC as PC,UC_M68K_REG_D0 as D0,UC_M68K_REG_SR as SR
 stop=0x300000;stack=0xffdf00
 u.mem_map(stop,0x1000);u.mem_map(0xc00000,0x1000)
 def call(name,args=(),count=30000000):
  u.reg_write(SR,0x2700);u.reg_write(SP,stack)
  u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
  u.emu_start(sym[name],stop,count=count)
  assert u.reg_read(PC)==stop,(name,'did not return',hex(u.reg_read(PC)))
  assert u.reg_read(SP)==stack+4,name+' changed stack'
  return u.reg_read(D0)&65535
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,n):u.mem_write(a,(n&65535).to_bytes(2,'big'))
 assert call('combat_contract')==0,'Target combat contract failed'
 assert sym['_BSS_ORIGIN']+sym['_BSS_LENGTH']<=0xffd000
 call('combat_begin_probe');call('combat_publish')
 assert word(sym['published'])==1
 frame=sym['frames'];count=word(frame+1312);assert 6<=count<=56
 snapshot=bytes(u.mem_read(frame,1962));call('combat_publish')
 assert bytes(u.mem_read(frame,1962))==snapshot,'Changed a published snapshot'
 writes=[];early=[False]
 def read(cpu,access,address,size,value,user):
  if address==0xc00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
  if address==0xc00008:cpu.mem_write(address,(0xe000 if early[0] else 0xe900).to_bytes(size,'big'))
 def write(cpu,access,address,size,value,user):
  if 0xc00000<=address<=0xc00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 call('vblank_user');assert not writes and word(sym['published'])==1
 early[0]=True;call('vblank_user')
 assert not word(sym['published']) and word(sym['submitted'])==1
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xc00000)
 assert actual==bytes(u.mem_read(frame,count*8))+bytes(u.mem_read(frame+1320,320))
 assert word(sym['upload_high'])==len(actual)<=768
 controls=[v for a,n,v in writes if a==0xc00004]
 assert len(controls)==5 and 0xc0000000 not in controls,'Unexpected VDP/CRAM operation'
 writes.clear();call('vblank_user');assert not writes
 # Force a won state through a narrow ABI adapter; panel is staged, not drawn
 # by simulation, and is cleared through the same callback on restart.
 call('combat_win_probe');call('combat_publish')
 winner=frame+(word(sym['published'])-1)*1962
 assert all(not (word(winner+i*8+4)&0x8000) for i in range(word(winner+1312))),'Actors can cover the opaque end panel'
 call('vblank_user')
 assert word(sym['shown_overlay'])==1 and word(sym['upload_high'])<=1088
 assert len([v for a,n,v in writes if a==0xc00000])>=160
 writes.clear();call('combat_begin_probe');call('combat_publish');call('vblank_user')
 assert not word(sym['shown_overlay'])
 # Finite fanfare uses changing notes and retires after its intended duration.
 call('lab_sound',(100,32,0));assert word(sym['fanfare'])>0
 for tick in range(101,281):call('lab_sound',(tick,0,0))
 assert not word(sym['fanfare'])
 writes.clear()
 put(frame+1312,57);put(sym['published'],1);call('vblank_user')
 assert not writes and word(sym['video_fault']) and not word(sym['published'])
 put(sym['published'],3);call('vblank_user');assert not writes and not word(sym['published'])
 u.hook_del(hr);u.hook_del(hw)
 return True

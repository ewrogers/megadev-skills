"""Execute timeline, gameplay consumer and frame publication on original 68000."""
def check_animation(u,sym):
 from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
 stop=0x300000;stack=0xFFDF00;model=0x210000
 u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
 def call(name,args=()):
  u.reg_write(UC_M68K_REG_A7,stack)
  u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((n&0xffffffff).to_bytes(4,'big') for n in args))
  u.emu_start(sym[name],stop,count=1000000)
  assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not finish'
  assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
  return u.reg_read(UC_M68K_REG_D0)&65535
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,n):u.mem_write(a,(n&65535).to_bytes(2,'big'))
 call('tr_init',(model,))
 for i in range(60):call('tr_advance',(model,5,8,0))
 expected=bytes(u.mem_read(model,48));assert word(model+18)==60 and word(model+38)==5
 call('tr_init',(model,))
 for i in range(50):call('tr_advance',(model,6,8,0))
 assert bytes(u.mem_read(model,48))==expected,'Target PAL/NTSC state mismatch'
 call('tr_init',(model,));put(model+10,812)
 call('tr_advance',(model,5,0,64))
 for i in range(23):call('tr_advance',(model,5,0,0))
 assert word(model+28)==1 and word(model+30)>1 and word(model+40)==1 and word(model+42)==1
 assert word(model)==0,'Completed attack did not return to idle'
 call('tr_advance',(model,5,0,64))
 for i in range(5):call('tr_advance',(model,5,0,0))
 hits=word(model+28);call('tr_advance',(model,5,0,16));assert word(model)==3
 box=model+512;assert call('an_hitbox',(model,0,203,144,box))==0
 for i in range(20):call('tr_advance',(model,5,0,0))
 assert word(model+28)==hits
 call('tr_advance',(model,1,2,16));call('tr_advance',(model,4,0,0));assert word(model)==4
 for i in range(30):call('tr_advance',(model,5,0,0))
 assert word(model+8)==1 and word(model+2)==2
 final=bytes(u.mem_read(model,10));completions=word(model+42)
 for i in range(30):call('tr_advance',(model,5,0,0))
 assert bytes(u.mem_read(model,10))==final and word(model+42)==completions
 call('tr_advance',(model,5,0,64));assert word(model)==5
 for i in range(24):call('tr_advance',(model,5,0,0))
 assert word(model)==0
 call('tr_advance',(model,5,0,128));frozen=bytes(u.mem_read(model,48))
 for i in range(50):call('tr_advance',(model,6,8,64))
 assert bytes(u.mem_read(model,48))==frozen,'Paused model advanced'
 call('tr_init',(model,));assert call('tr_advance',(model,65535,0,64))==4
 assert word(model+46)==1 and word(model)==2 and word(model+4)==3
 # Checked public API preserves the running clip on an invalid selection.
 before=bytes(u.mem_read(model,10));assert call('an_play',(model,6,1))==0x2000
 assert bytes(u.mem_read(model,10))==before
 # The standalone TOUR goes through actual consumer transitions, not synthetic counts.
 if 'animation_tour' in sym:
  call('tr_init',(sym['training'],))
  for i in range(200):
   call('tr_advance',(sym['training'],5,0,0));call('animation_tour')
   if word(sym['tour_ok']):break
  else:raise AssertionError('Compiled animation tour did not finish')
  assert word(sym['training']+28)==1 and word(sym['training']+20)==1
 call('tr_init',(sym['training'],));call('animation_publish')
 frame=sym['frames'];count=word(frame+1312);assert 1<=count<=16 and word(sym['published'])==1
 snapshot=bytes(u.mem_read(frame,1640));call('animation_publish');assert bytes(u.mem_read(frame,1640))==snapshot
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
 expected=bytes(u.mem_read(frame,count*8))+bytes(u.mem_read(frame+1320,320))
 assert actual==expected and len(actual)<=448
 assert word(sym['upload_high'])==len(actual)
 controls=[v for a,n,v in writes if a==0xC00004]
 assert len(controls)==5 and 0xC0000000 not in controls
 writes.clear();call('vblank_user');assert not writes
 put(frame+1312,17);put(sym['published'],1);call('vblank_user')
 assert not writes and word(sym['video_fault'])==1 and not word(sym['published'])
 u.hook_del(hr);u.hook_del(hw)
 return True

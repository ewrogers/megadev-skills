"""Run actual original-68000 effect model, SAT admission and VBlank consumer."""
import struct

def check_effects(u,sym):
 from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
 stop=0x300000;stack=0xFFDF00;p=0x210000;view=p+1024
 u.mem_map(stop,4096);u.mem_map(0xC00000,4096)
 def call(name,args=(),limit=3000000):
  u.reg_write(UC_M68K_REG_A7,stack);u.mem_write(stack,struct.pack('>I',stop)+b''.join(struct.pack('>I',n&0xffffffff) for n in args))
  u.emu_start(sym[name],stop,count=limit)
  assert u.reg_read(UC_M68K_REG_PC)==stop,name+' unfinished'
  assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' stack changed'
  return u.reg_read(UC_M68K_REG_D0)&65535
 def put(a,n):u.mem_write(a,struct.pack('>H',n&65535))
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def reset():u.mem_write(p,b'\0'*792);call('fx_init',(p,))
 reset()
 for _ in range(60):call('fx_advance',(p,5,8,0))
 expected=bytes(u.mem_read(p,792));assert word(p+772)==60
 reset()
 for _ in range(50):call('fx_advance',(p,6,8,0))
 assert bytes(u.mem_read(p,792))==expected,'PAL/NTSC simulation diverged'
 call('fx_advance',(p,6,0,128));frozen=bytes(u.mem_read(p,792))
 for _ in range(10):call('fx_advance',(p,6,8,64))
 assert bytes(u.mem_read(p,792))==frozen
 reset();call('fx_burst',(p,));assert call('fx_alive',(p,0,24))==12
 # First spark's exact position, gravity and lifetime after one 60Hz step.
 x,y=word(p+768),word(p+770);call('fx_advance',(p,5,0,0))
 assert struct.unpack('>4h2H',u.mem_read(p,12))==(x*16-40,y*16,-40,2,31,1)
 for _ in range(4):call('fx_burst',(p,))
 assert call('fx_alive',(p,0,24))==24 and word(p+788)>0
 for _ in range(52):call('fx_advance',(p,5,0,0))
 assert not call('fx_alive',(p,0,24))
 assert call('fx_advance',(p,65535,0,0))==4 and word(p+790)==1
 # Fill the SAT with geometrically spaced effects; then force a scanline overload.
 reset()
 for i in range(64):u.mem_write(p+i*12,struct.pack('>4h2H',(12+i%12*25)*16,(50+i//12*26)*16,0,0,20,1 if i<24 else 3 if i<32 else 5))
 before=bytes(u.mem_read(p,792));call('fx_build',(p,view));assert bytes(u.mem_read(p,792))==before
 assert word(view+448)==56 and word(view+450)>0
 for i in range(56):assert word(view+i*8+2)&127==(i+1 if i<55 else 0)
 for i in range(64):put(p+i*12,50*16);put(p+i*12+2,96*16)
 call('fx_build',(p,view));assert word(view+448)<=17 and word(view+450)>30
 assert max(u.mem_read(view+458,28))<=16
 for i in range(64):put(p+i*12,-128)
 call('fx_build',(p,view));assert word(view+448)==1 and word(view+452)==64
 # Snapshot publication remains immutable until its consumer retires it.
 game=sym['world'];u.mem_write(game,b'\0'*792);call('fx_init',(game,));call('fx_burst',(game,))
 put(sym['hz'],60);put(sym['published'],0);call('effects_publish');pending=word(sym['published']);assert pending in (1,2)
 frame=sym['frames']+(pending-1)*806;count=word(frame+448);assert count==13
 snapshot=bytes(u.mem_read(frame,806));call('fx_advance',(game,5,8,64));call('effects_publish');assert bytes(u.mem_read(frame,806))==snapshot
 writes=[];early=False
 def read(cpu,access,a,size,value,user):
  if a==0xC00004:cpu.mem_write(a,(8).to_bytes(size,'big'))
  if a==0xC00008:cpu.mem_write(a,(0xE000 if early else 0xE900).to_bytes(size,'big'))
 def write(cpu,access,a,size,v,user):
  if 0xC00000<=a<=0xC00007:writes.append((a,size,v&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 call('vblank_user');assert not writes and word(sym['published'])==pending
 early=True;call('vblank_user');assert not word(sym['published']) and word(sym['submitted'])==1
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 sx,sy=struct.unpack('>2h',u.mem_read(frame+454,4))
 expected=bytes(u.mem_read(frame,count*8))+struct.pack('>4H',(-sy)&65535,(-sy)&65535,sx&65535,sx&65535)+bytes(u.mem_read(frame+486,320))
 assert actual==expected and len(actual)<=776 and word(sym['upload_high'])==len(actual)
 assert not any(a==0xC00004 and n==4 and v==0xC0000000 for a,n,v in writes),'Ordinary frame wrote CRAM'
 writes.clear();call('vblank_user');assert not writes
 put(frame+448,57);put(sym['published'],pending);call('vblank_user');assert not writes and word(sym['video_fault'])==1
 put(sym['video_fault'],0);put(sym['published'],3);call('vblank_user');assert not writes and word(sym['video_fault'])==1
 u.hook_del(hr);u.hook_del(hw)
 if 'effects_tour' in sym:call('effects_tour',limit=30000000);assert word(sym['tour_ok'])==1
 return True

"""Execute the production queries and immutable VBlank submission on base 68000."""
import random
import struct
from fractions import Fraction
from test_collision import oracle

def check_collision(u,sym):
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
 # Fixed reproducer for the optimized inline-broad regression (strictly inside).
 u.mem_write(model,struct.pack('>2h2h4h',-2077,-2800,-3019,64,-3629,-3561,3013,971))
 call('cp_ray',(model,model+4,model+8,model+32));assert word(model+32)==2
 rng=random.Random(68000)
 # Independently generated inputs exercise target-only word multiply/divide,
 # signed restoration, both moving bodies, range edges and point segments.
 for i in range(1200):
  ax,ay=[rng.randint(-16384,16384) for _ in range(2)]
  bx=max(-16384,min(16384,ax+rng.randint(-5000,5000)));by=max(-16384,min(16384,ay+rng.randint(-5000,5000)))
  aw,ah,bw,bh=[rng.randint(1,4096) for _ in range(4)]
  dx,dy,vx,vy=[rng.randint(-4096,4096) for _ in range(4)]
  dx=max(-16384-ax,min(16384-ax,dx));dy=max(-16384-ay,min(16384-ay,dy))
  vx=max(-16384-bx,min(16384-bx,vx));vy=max(-16384-by,min(16384-by,vy))
  point=i%2;row=(ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy,point)
  u.mem_write(model,struct.pack('>4h2h4h2h',ax,ay,aw,ah,dx,dy,bx,by,bw,bh,vx,vy))
  if point:call('cp_ray',(model,model+8,model+12,model+32))
  else:call('cp_sweep',(model,model+8,model+12,model+20,model+32))
  kind,time,index,nx,ny,num,den=struct.unpack('>3H2h2i',u.mem_read(model+32,18))
  want=oracle(row);assert (kind,time,nx,ny)==want[:4],(row,(kind,time,nx,ny),want)
  if kind==1:assert Fraction(num,den)==want[4]
 # Simultaneous corner walls block both axes in one contact; tangent survives.
 u.mem_write(model,struct.pack('>4h2h8h',0,0,16,16,128,96,80,-64,16,256,-64,80,256,16))
 call('cp_slide',(model,model+8,model+12,2,model+64))
 out=struct.unpack('>4h2H2h2H2h',u.mem_read(model+64,24))
 assert out[:6]==(64,64,16,16,1,2) and out[6:8]==(0,0),out
 call('cd_init',(model,2))
 for i in range(60):call('cd_advance',(model,5,8,0))
 expected=bytes(u.mem_read(model,74));assert word(model+60)==60
 call('cd_init',(model,2))
 for i in range(50):call('cd_advance',(model,6,8,0))
 assert bytes(u.mem_read(model,74))==expected,'Target PAL/NTSC mismatch'
 call('cd_advance',(model,5,0,128));frozen=bytes(u.mem_read(model,74))
 for i in range(50):call('cd_advance',(model,6,8,0))
 assert bytes(u.mem_read(model,74))==frozen,'Paused model advanced'
 call('cd_init',(model,0));call('cd_tick',(model,0))
 assert word(model)==168*16 and word(model+8)==158*16 and word(model+28)==1
 call('cd_init',(model,2));assert call('cd_advance',(model,65535,8,0))==4 and word(model+70)==1
 if 'collision_tour' in sym:
  call('cd_init',(sym['demo'],0))
  for i in range(200):
   call('cd_advance',(sym['demo'],5,0,0));call('collision_tour')
   if word(sym['tour_ok']):break
  else:raise AssertionError('Compiled collision tour did not finish')
 call('cd_init',(sym['demo'],0));call('collision_publish')
 frame=sym['frames'];count=word(frame+1312);assert 1<=count<=32 and word(sym['published'])==1
 snapshot=bytes(u.mem_read(frame,1640));call('collision_publish');assert bytes(u.mem_read(frame,1640))==snapshot
 writes=[];early=True
 def read(cpu,access,address,size,value,user):
  if address==0xC00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
  if address==0xC00008:cpu.mem_write(address,(0xE000 if early else 0xE900).to_bytes(size,'big'))
 def write(cpu,access,address,size,value,user):
  if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 # Ordinary movement must never reinstall a map or write VDP from foreground.
 # Exercise the exact production update seam, not only the geometry model.
 writes.clear();put(sym['published'],0);put(sym['hz'],60)
 for mode,map_id in [(1,2),(2,3)]:
  call('cd_init',(sym['demo'],mode));put(sym['drawn_map'],map_id)
  for i in range(20):
   assert call('collision_update',(1,8 if i<10 else 2,0))==0
   assert not writes,'Movement touched VDP or reinstalled the static map'
   put(sym['published'],0)
 call('cd_init',(sym['demo'],0));put(sym['drawn_map'],0);call('collision_publish')
 frame=sym['frames']+word(sym['published'])*1640-1640
 count=word(frame+1312);pending=word(sym['published'])
 early=False;call('vblank_user');assert not writes and word(sym['published'])==pending
 early=True;call('vblank_user');assert not word(sym['published']) and word(sym['submitted'])==1
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 expected=bytes(u.mem_read(frame,count*8))+bytes(u.mem_read(frame+1320,320))
 assert actual==expected and len(actual)<=576
 assert word(sym['upload_high'])==len(actual)
 controls=[v for a,n,v in writes if a==0xC00004]
 assert len(controls)==5 and 0xC0000000 not in controls
 writes.clear();call('vblank_user');assert not writes
 put(frame+1312,33);put(sym['published'],pending);call('vblank_user')
 assert not writes and word(sym['video_fault'])==1 and not word(sym['published'])
 # Raising/lowering the wall stages one column without blanking or touching
 # VDP in foreground. The paired patch survives a late blank unchanged.
 put(sym['video_fault'],0);writes.clear()
 call('cd_init',(sym['demo'],0));put(sym['drawn_map'],0)
 assert call('collision_update',(1,0,1))==0 and not writes
 assert word(sym['wall_pending'])==1
 words=bytes(u.mem_read(sym['wall_words'],48));pending=word(sym['published'])
 early=False;call('vblank_user');assert not writes and word(sym['wall_pending'])==1
 assert bytes(u.mem_read(sym['wall_words'],48))==words
 early=True;call('vblank_user');assert not word(sym['wall_pending']) and not word(sym['published'])
 patch_data=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 assert patch_data[-48:]==words and len(patch_data)<=624
 assert len([v for a,n,v in writes if a==0xC00004])==29
 u.hook_del(hr);u.hook_del(hw)
 return True

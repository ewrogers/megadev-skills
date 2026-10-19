"""Execute compiled original-M68000 arithmetic, motion and immutable publication."""
import math
import random
import struct

def check_math(u,sym):
 from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
 stop=0x300000;stack=0xFFDF00;p=0x210000
 u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
 def call(name,args=()):
  u.reg_write(UC_M68K_REG_A7,stack);u.mem_write(stack,struct.pack('>I',stop)+b''.join(struct.pack('>I',n&0xffffffff) for n in args))
  u.emu_start(sym[name],stop,count=2000000)
  assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not finish'
  assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
  return u.reg_read(UC_M68K_REG_D0)&0xffffffff
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def signed(a):return struct.unpack('>h',u.mem_read(a,2))[0]
 def put(a,n):u.mem_write(a,(n&65535).to_bytes(2,'big'))
 def trunc(n,d):return (-1 if n<0 else 1)*(abs(n)//d)
 rng=random.Random(704);edge=[-32768,-32767,-257,-256,-1,0,1,255,256,32767]
 for a,b in [(a,b) for a in edge for b in edge]+[(rng.randint(-32768,32767),rng.randint(-32768,32767)) for _ in range(180)]:
  for name,expected in [('add',a+b),('sub',a-b),('mul',trunc(a*b,256)),('div',trunc(a*256,abs(b))*(-1 if b<0 else 1) if b else None)]:
   put(p,12345);ok=call('mm_'+name,(a,b,p))&65535;valid=expected is not None and -32768<=expected<=32767
   assert bool(ok)==valid and signed(p)==(expected if valid else 12345),(name,a,b,ok,signed(p),expected)
 for n in [-32768,-129,-128,-1,0,127,128,32767]:
  put(p,12345);ok=call('mm_from_int',(n,p))&65535
  assert bool(ok)==(-128<=n<=127) and signed(p)==(n*256 if ok else 12345)
  assert call('mm_to_int',(n,))&65535==trunc(n,256)&65535
 for name in ['add','sub','mul','div']:assert not call('mm_'+name,(256,256,0))&65535
 for n in [0,1,2,3,4,65535,65536,2147483648,4294967295]+[rng.randrange(1<<32) for _ in range(180)]:assert call('mm_isqrt',(n,))&65535==math.isqrt(n)
 for angle in range(1024):
  raw=call('mm_sin',(angle,))&65535;value=raw if raw<32768 else raw-65536
  assert abs(value-math.sin(angle*math.tau/1024)*256)<=0.501
  assert call('mm_sin',(angle+1024,))&65535==raw
  assert call('mm_cos',((angle-256)&65535,))&65535==raw
 for x,y in [(0,0),(1,1),(-1,1),(1,0),(-32768,-32768),(32767,1)]+[(rng.randint(-32768,32767),rng.randint(-32768,32767)) for _ in range(100)]:
  u.mem_write(p,struct.pack('>2h',x,y));u.mem_write(p+8,struct.pack('>2h',123,45))
  assert call('mm_length2',(p,))==x*x+y*y
  ok=call('mm_normalize',(p,p+8))&65535;result=struct.unpack('>2h',u.mem_read(p+8,4))
  assert bool(ok)==bool(x or y)
  if not ok:assert result==(123,45)
  else:
   length=math.hypot(x,y);assert all(abs(a-b*256/length)<=1.02 for a,b in zip(result,(x,y)))
   assert call('mm_normalize',(p,p))&65535 and bytes(u.mem_read(p,4))==bytes(u.mem_read(p+8,4))
 for _ in range(160):
  a=[rng.randint(-32768,32767) for _ in range(2)];b=[rng.randint(-32768,32767) for _ in range(2)]
  u.mem_write(p,struct.pack('>4h',*a,*b));u.mem_write(p+8,struct.pack('>i',12345));expected=sum(x*y for x,y in zip(a,b))
  ok=call('mm_dot',(p,p+4,p+8))&65535
  assert bool(ok)==(-2147483648<=expected<=2147483647)
  assert struct.unpack('>i',u.mem_read(p+8,4))[0]==(expected if ok else 12345)
 # Only this extreme pair overflows the sum of two signed-word products.
 u.mem_write(p,struct.pack('>4h',-32768,-32768,-32768,-32768));u.mem_write(p+8,struct.pack('>i',12345))
 assert not call('mm_dot',(p,p+4,p+8))&65535 and int.from_bytes(u.mem_read(p+8,4),'big')==12345
 for x,y,angle in [(256,0,256),(-32768,-32768,128),(32767,32767,128),(1,1,0)]+[(rng.randint(-32768,32767),rng.randint(-32768,32767),rng.randrange(1024)) for _ in range(100)]:
  def trig(i):
   n=call('mm_sin',(i,))&65535;return n if n<32768 else n-65536
  s=trig(angle);c=trig(angle+256);expected=(trunc(x*c-y*s,256),trunc(x*s+y*c,256));valid=all(-32768<=v<=32767 for v in expected)
  u.mem_write(p,struct.pack('>2h',x,y));u.mem_write(p+8,struct.pack('>2h',123,45));ok=call('mm_rotate',(p,angle,p+8))&65535
  assert bool(ok)==valid and struct.unpack('>2h',u.mem_read(p+8,4))==(expected if valid else (123,45))
  if valid:assert call('mm_rotate',(p,angle,p))&65535 and bytes(u.mem_read(p,4))==bytes(u.mem_read(p+8,4))
 for t in range(258):
  for a,b in [(-32768,32767),(32767,-32768),(-123,456)]:
   put(p,12345);ok=call('mm_lerp',(a,b,t,p))&65535
   assert bool(ok)==(t<=256) and signed(p)==(a+trunc((b-a)*t,256) if ok else 12345)
  put(p,999);ok=call('mm_smoothstep',(t,p))&65535
  assert bool(ok)==(t<=256) and word(p)==((t*t*(768-2*t))//65536 if ok else 999)
 game=sym['motion'];call('mt_init',(game,));put(game+8,1)
 for _ in range(60):call('mt_advance',(game,5,0,0))
 expected=bytes(u.mem_read(game,30));call('mt_init',(game,));put(game+8,1)
 for _ in range(50):call('mt_advance',(game,6,0,0))
 assert bytes(u.mem_read(game,30))==expected and word(game+14)==60 and not word(game+26)
 call('mt_advance',(game,6,0,128));frozen=bytes(u.mem_read(game,30))
 for _ in range(30):call('mt_advance',(game,6,8,0))
 assert bytes(u.mem_read(game,30))==frozen
 call('mt_init',(game,));assert call('mt_advance',(game,65535,0,0))&65535==4 and word(game+28)==1
 for view in range(3):
  call('mt_init',(game,));put(game+8,view)
  for _ in range(400):call('mt_advance',(game,5,0,0))
  assert not word(game+26)
 call('mt_init',(game,));call('mt_advance',(game,5,0,0));angle=word(game+12)
 call('mt_advance',(game,5,0,64));assert word(game+12)==(angle+1022)&1023
 put(game+8,2);put(game+20,100);put(game+22,0);call('mt_advance',(game,5,0,64));assert word(game+20)==98
 call('mt_init',(game,));put(game+8,1)
 for _ in range(600):call('mt_advance',(game,5,8,0))
 assert word(game)==56*256 and not word(game+26)
 for name,args in [('normalize',(0,p)),('normalize',(p,0)),('rotate',(0,0,p)),('rotate',(p,0,0)),('dot',(0,p,p+8)),('dot',(p,0,p+8)),('dot',(p,p,0))]:assert not call('mm_'+name,args)&65535
 if 'math_tour' in sym:call('math_tour');assert word(sym['tour_ok'])==1
 call('mt_init',(game,));put(sym['hz'],60);put(sym['published'],0)
 call('math_publish');pending=word(sym['published']);assert pending in (1,2)
 frame=sym['frames']+(pending-1)*1640;count=word(frame+1312);assert 1<=count<=28
 snapshot=bytes(u.mem_read(frame,1640));call('math_publish');assert bytes(u.mem_read(frame,1640))==snapshot
 writes=[];early=False
 def read(cpu,access,address,size,value,user):
  if address==0xC00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
  if address==0xC00008:cpu.mem_write(address,(0xE000 if early else 0xE900).to_bytes(size,'big'))
 def write(cpu,access,address,size,value,user):
  if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 call('vblank_user');assert not writes and word(sym['published'])==pending
 early=True;call('vblank_user');assert not word(sym['published']) and word(sym['submitted'])==1
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 assert actual==bytes(u.mem_read(frame,count*8))+bytes(u.mem_read(frame+1320,320)) and len(actual)<=544
 assert word(sym['upload_high'])==len(actual)
 assert not any(a==0xC00004 and n==4 and v==0xC0000000 for a,n,v in writes),'Ordinary frame wrote CRAM'
 writes.clear();call('vblank_user');assert not writes
 put(frame+1312,29);put(sym['published'],pending);call('vblank_user');assert not writes and word(sym['video_fault'])==1 and not word(sym['published'])
 put(sym['video_fault'],0);put(sym['published'],3);call('vblank_user');assert not writes and word(sym['video_fault'])==1
 u.hook_del(hr);u.hook_del(hw)
 return True

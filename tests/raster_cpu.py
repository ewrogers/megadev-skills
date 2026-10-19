"""Execute original-68000 raster model, setup, Gate Array vector and H handler.
VDP flags/scanlines and BIOS DMA are modeled; this does not prove bus timing.
"""
import struct

def check_raster(u,sym,demo=False):
 from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 import unicorn.m68k_const as r
 stop=0x300000;stack=0xFFDF00;p=0x210000;view=p+1024
 u.mem_map(stop,4096);u.mem_map(0xC00000,4096)
 def call(name,args=(),limit=3000000):
  u.reg_write(r.UC_M68K_REG_A7,stack);u.mem_write(stack,struct.pack('>I',stop)+b''.join(struct.pack('>I',a&0xffffffff) for a in args));u.emu_start(sym[name],stop,count=limit)
  assert u.reg_read(r.UC_M68K_REG_PC)==stop,name+' unfinished'
  assert u.reg_read(r.UC_M68K_REG_A7)==stack+4,name+' stack changed'
  return u.reg_read(r.UC_M68K_REG_D0)&65535
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,n):u.mem_write(a,struct.pack('>H',n&65535))
 def reset(d):u.mem_write(p,b'\0'*30);call('rt_init',(p,d))
 for d in (0,1):
  reset(d)
  for i in range(60):call('rt_advance',(p,5,8,32 if i==0 else 0))
  expected=bytes(u.mem_read(p,30));reset(d)
  for i in range(50):call('rt_advance',(p,6,8,32 if i==0 else 0))
  assert bytes(u.mem_read(p,30))==expected and word(p+10)==60
  call('rt_advance',(p,6,0,128));frozen=bytes(u.mem_read(p,30));call('rt_advance',(p,20,15,112));assert bytes(u.mem_read(p,30))==frozen
  reset(d);assert call('rt_advance',(p,65535,0,0))==4 and word(p+28)==1
 reset(0)
 for line in [112,113,127,128,143,144,175,176]:
  put(p+8,line);call('rt_build',(p,view));assert call('rt_plan_valid',(view,))
  words=struct.unpack('>10H',u.mem_read(view,20));assert words[0]==line and words[2]==(255-2*line)&255
  for y in [line,223]:assert 255-((y+words[2])&255)==2*line-y
  count=word(view+1060);assert 10<=count<=18
  sat=struct.unpack('>'+str(count*4)+'H',u.mem_read(view+916,count*8))
  for i in range(count):assert sat[i*4+1]&127==(i+1 if i+1<count else 0)
  cells=[sat[i*4:i*4+4] for i in range(10,count)]
  originals={c[2]&2047:c for c in cells if not c[2]&0x1000}
  reflected_dx=[]
  for c in cells:
   if c[2]&0x1000:
    source=originals[c[2]&2047]
    assert c[0]-128==2*line-(source[0]-128)-8 and c[0]-128>=line+6
    reflected_dx.append(c[3]-source[3])
  assert len(set(reflected_dx))<=1,'Reflected actor cells tore apart'
  for y in range(224):
   widths=[(((sat[i*4+1]>>10)&3)+1)*8 for i in range(count) if sat[i*4]-128<=y<sat[i*4]-128+(((sat[i*4+1]>>8)&3)+1)*8]
   assert len(widths)<=20 and sum(widths)<=320
 put(view,111);assert not call('rt_plan_valid',(view,))
 assert not call('rt_plan_valid',(0,))
 reset(1);call('rt_build',(p,view));top=bytes(u.mem_read(view+20,512));bottom=bytes(u.mem_read(view+532,384))
 call('rt_advance',(p,5,8,0));call('rt_build',(p,view));assert bytes(u.mem_read(view+20,512))!=top and bytes(u.mem_read(view+532,384))==bottom
 reset(1);call('rt_advance',(p,0,0,16));call('rt_build',(p,view))
 assert not word(view+14) and word(view+1060)<=2
 scroll=struct.unpack('>448H',u.mem_read(view+20,896));assert len(set(scroll))==1,'Split-off left a second horizontal camera active'
 # Gate Array low word + full Work RAM vector, without bit opcodes on GA.
 handler=sym['raster_hint'];assert 0xFF0000<=handler<0xFFF700 and not handler&1
 put(0xA12006,0xFD0C);u.mem_write(0xFFFD0E,struct.pack('>I',0xFFABC0));call('raster_install')
 assert word(0xA12006)==handler&65535 and int.from_bytes(u.mem_read(0xFFFD0E,4),'big')==handler
 call('raster_install');put(sym['raster_armed'],12);call('raster_detach')
 assert word(0xA12006)==0xFD0C and int.from_bytes(u.mem_read(0xFFFD0E,4),'big')==0xFFABC0 and not word(sym['raster_armed'])
 call('raster_detach');assert not word(sym['raster_installed'])
 # Real foreground snapshot and VBlank C/assembly DMA adapter.
 game=sym['world'];call('rt_init',(game,int(demo)));put(sym['hz'],60);put(sym['published'],0);call('raster_publish');pending=word(sym['published']);assert pending in (1,2)
 frame=sym['frames']+(pending-1)*1574;count=word(frame+1060);assert count==(12 if demo else 18)
 snapshot=bytes(u.mem_read(frame,1574));call('rt_advance',(game,5,8,32));call('raster_publish');assert bytes(u.mem_read(frame,1574))==snapshot
 writes=[];dmas=[];events=[];hv_reads=0;late_start=False;late_end=False
 def read(cpu,access,a,size,value,user):
  nonlocal hv_reads
  if a==0xC00008:
   hv_reads+=1;line=24 if late_start or (late_end and hv_reads>1) else 224 if hv_reads==1 else 248
   cpu.mem_write(a,(line<<8).to_bytes(size,'big'))
  if a==0xC00004:cpu.mem_write(a,(0 if late_start or (late_end and hv_reads>1) else 8).to_bytes(size,'big'))
 def write(cpu,access,a,size,value,user):
  if 0xC00000<=a<=0xC00007:
   entry=(a,size,value&((1<<(size*8))-1));writes.append(entry);events.append(entry)
 def dma(cpu,a,size,user):
  if a!=0x2D0:return
  dest=cpu.reg_read(r.UC_M68K_REG_D0);src=cpu.reg_read(r.UC_M68K_REG_D1)&0xFFFFFF;words=cpu.reg_read(r.UC_M68K_REG_D2)&65535
  expected_dma=[(0x7C000002,frame+20,448),(0x78000002,frame+916,count*4),(0x60000002,frame+1062,256)]
  assert (dest,src,words) in expected_dma,(hex(dest),hex(src),words)
  dmas.append(bytes(cpu.mem_read(src,words*2)));events.append(('DMA',dest,words))
  # BIOS's documented clobbers; C ABI bridge must protect callee-saved state.
  for reg in [r.UC_M68K_REG_D0,r.UC_M68K_REG_D1,r.UC_M68K_REG_D2,r.UC_M68K_REG_D3,r.UC_M68K_REG_A6]:cpu.reg_write(reg,0x12345678)
 u.mem_write(0x2D0,b'\x4E\x75');hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write);hd=u.hook_add(UC_HOOK_CODE,dma);u.ctl_flush_tb()
 call('vblank_user');assert not word(sym['published']) and word(sym['submitted'])==1
 plan=bytes(u.mem_read(frame,20));assert bytes(u.mem_read(sym['raster_plan'],20))==plan
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 expected=struct.pack('>4H',0x4CE,0x28E,*struct.unpack('>2H',plan[16:20]))
 assert actual==expected and dmas==[snapshot[20:916],snapshot[916:916+count*8],snapshot[1062:1574]]
 assert word(sym['raster_armed'])==word(sym['raster_epoch'])!=0
 assert word(sym['upload_high'])==896+count*8+512+8
 # Programming the H counter must precede bulk work; palette restore precedes it too.
 counter=next(i for i,e in enumerate(events) if e[0]==0xC00004 and e[1]==2 and e[2]>>8==0x8A)
 assert 0<counter<next(i for i,e in enumerate(events) if e[0]=='DMA')
 writes.clear();dmas.clear();hv_reads=0;late_start=True;put(sym['published'],pending);call('vblank_user')
 assert not writes and not dmas and not word(sym['raster_armed']) and word(sym['published'])==pending
 late_start=False;late_end=True;hv_reads=0;call('vblank_user');assert not word(sym['raster_armed']) and word(sym['setup_late'])>=2
 late_end=False;hv_reads=0;call('vblank_user');assert word(sym['raster_armed'])==word(sym['raster_epoch'])!=0,'No recovery after late setup'
 u.hook_del(hr);u.hook_del(hw);u.hook_del(hd)
 # Measured snapshots, not HBlank wakeups or simulation steps.
 for hz in [50,60]:
  put(sym['hz'],hz);put(sym['window_start'],0);put(sym['last_submitted'],0);put(sym['submitted'],hz)
  call('raster_measure',(hz,));assert word(sym['frame_rate'])==hz
  put(sym['submitted'],hz+hz//2);call('raster_measure',(hz*2,));assert word(sym['frame_rate'])==hz//2
  call('raster_measure',(hz*3,));assert word(sym['frame_rate'])==0
 # Actual Level-4 entry/exit: model flags and counters, never timing/cycles.
 regs=[getattr(r,'UC_M68K_REG_'+n) for n in ['D0','D1','D2','D3','D4','D5','D6','D7','A0','A1','A2','A3','A4','A5','A6']]
 def hint(flags=3,vblank=False,line=144,stuck=None,epoch_change=False):
  payload=(144,flags,223,0,0x8220 if flags&1 else 0x8238,0xE84,0xA42,1,0,0)
  u.mem_write(sym['raster_plan'],struct.pack('>10H',*payload));put(sym['raster_epoch'],123);put(sym['raster_armed'],123);put(sym['raster_reg_off'],0x8004)
  for n in ['raster_late','raster_timeout','raster_done']:put(sym[n],0)
  status_count=0;events=[];injected=False
  saved=[0x1000+i*0x101 for i in range(len(regs))]
  for reg,value in zip(regs,saved):u.reg_write(reg,value)
  u.reg_write(r.UC_M68K_REG_A7,stack)
  def io_read(cpu,access,a,size,value,user):
   nonlocal status_count
   if a==0xC00004:
    status_count+=1
    value=8 if vblank else (4 if stuck==1 else 0) if stuck is not None else (4 if status_count in (4,7) else 0)
    cpu.mem_write(a,value.to_bytes(size,'big'))
    if status_count>=3:assert cpu.reg_read(r.UC_M68K_REG_SR)&0x700==0x700,'Edge polling not atomic'
   if a==0xC00008:cpu.mem_write(a,(line<<8).to_bytes(size,'big'))
  def io_write(cpu,access,a,size,value,user):
   if 0xC00000<=a<=0xC00007:events.append((a,size,value&((1<<(size*8))-1)))
  def preempt(cpu,a,size,user):
   nonlocal injected
   if epoch_change and not injected and any(e[0]==0xC00000 for e in events) and cpu.reg_read(r.UC_M68K_REG_SR)&0x700==0x400:
    put(sym['raster_epoch'],124);put(sym['raster_armed'],124);injected=True
  h1=u.hook_add(UC_HOOK_MEM_READ,io_read);h2=u.hook_add(UC_HOOK_MEM_WRITE,io_write);h3=u.hook_add(UC_HOOK_CODE,preempt);u.ctl_flush_tb()
  u.emu_start(handler,sym['raster_hint_return'],count=30000)
  assert u.reg_read(r.UC_M68K_REG_PC)==sym['raster_hint_return'] and u.reg_read(r.UC_M68K_REG_A7)==stack
  assert [u.reg_read(reg) for reg in regs]==saved,'H handler leaked registers'
  assert bytes(u.mem_read(sym['raster_hint_return'],2))==b'\x4e\x73'
  u.hook_del(h1);u.hook_del(h2);u.hook_del(h3)
  return events,status_count
 events,n=hint();assert n==7 and word(sym['raster_done'])==1 and not word(sym['raster_timeout'])
 data=[e[2] for e in events if e[0]==0xC00000];assert data==[223<<16,0x0E840A42]
 assert (0xC00004,4,0xC0680000) in events
 events,n=hint(flags=0);assert word(sym['raster_done'])==1 and not any(e[2]==0xC0680000 for e in events)
 events,n=hint(vblank=True,line=0);assert not events and word(sym['raster_armed'])==123
 for early in [0,28,255]:
  events,n=hint(line=early);assert not events and word(sym['raster_armed'])==123,'Pending/early request consumed new plan'
 events,n=hint(line=160);assert word(sym['raster_late'])==1 and not any(e[0]==0xC00000 for e in events)
 for stuck in [0,1]:
  events,n=hint(stuck=stuck);assert n<=131 and word(sym['raster_timeout'])==1 and not any(e[0]==0xC00000 for e in events),(stuck,n,word(sym['raster_timeout']),events)
 events,n=hint(epoch_change=True);assert word(sym['raster_armed'])==124 and not any(e[2]==0xC0680000 for e in events),'Old H handler overwrote a rearmed frame'
 if 'raster_tour' in sym:put(sym['hz'],50 if demo else 60);call('raster_tour',limit=30000000);assert word(sym['tour_ok'])==1
 return True

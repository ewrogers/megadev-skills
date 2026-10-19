"""Execute Cinema's compiled Main/Sub adapters and publisher on original M68000.

CDC, Word RAM arbitration, DMA and PCM registers are modeled; this does not
measure physical drive latency, pixel output, PCM quality or DMA bus timing.
"""
from startup import check,symbols

def check_video(project,hz):
 import struct
 from unicorn import m68k_const as mc
 from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7 as SP,UC_M68K_REG_PC as PC,UC_M68K_REG_SR as SR,UC_M68K_REG_D0 as D0
 contexts={}
 for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
  sym=symbols((project/'build'/(file+'.elf.sym')).read_text())
  def collect(u,s,kind=kind):contexts[kind]=(u,s);return True
  check((project/'disc'/file).read_bytes(),sym,kind,collect)
 main,ms=contexts['main'];sub,ss=contexts['sub']
 main.mem_map(0xc00000,0x1000);sub.mem_map(0x80000,0x40000)
 boot=symbols((project/'build/sp.bin.elf.sym').read_text());sub.mem_write(0x6000,(project/'build/sp.bin').read_bytes())
 for u,s in contexts.values():u.mem_map(0x300000,0x1000)
 def get(u,a,n=2):return int.from_bytes(u.mem_read(a,n),'big')
 def put(u,a,v,n=2):u.mem_write(a,(v&((1<<(n*8))-1)).to_bytes(n,'big'))
 def ret(u,value=0):
  sp=u.reg_read(SP);u.reg_write(D0,value);u.reg_write(PC,get(u,sp,4));u.reg_write(SP,sp+4)
 def call(kind,name,args=()):
  u,s=contexts[kind];stack=0xffdf00 if kind=='main' else 0x1ff00
  u.reg_write(SR,0x2700);u.reg_write(SP,stack);u.mem_write(stack,(0x300000).to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
  try:u.emu_start(s[name],0x300000,count=4000000)
  except Exception as e:raise AssertionError(f'{kind} {name} at {u.reg_read(PC):x}') from e
  assert u.reg_read(PC)==0x300000 and u.reg_read(SP)==stack+4,(name,'did not return')
  return u.reg_read(D0)&65535
 dma=[];regs=[];vertical=[224];blank=[8]
 def reads(u,access,a,size,value,user):
  if a==0xc00004:put(u,a,blank[0],size)
  if a==0xc00008:put(u,a,vertical[0]<<8,size)
 def writes(u,access,a,size,value,user):
  if a==0xc00004 and size==2 and value&0xc000==0x8000:regs.append(value&65535)
 main.hook_add(UC_HOOK_MEM_READ,reads,begin=0xc00004,end=0xc00009)
 main.hook_add(UC_HOOK_MEM_WRITE,writes,begin=0xc00004,end=0xc00007)
 def main_code(u,a,size,user):
  if a in [ms['lab_display_on'],ms['lab_display_off'],ms['lab_clear']]:ret(u)
  if a==ms['lab_upload_work']:
   sp=u.reg_read(SP);src=get(u,sp+4,4);command=get(u,sp+8,4);words=get(u,sp+12,4)
   dma.append((src,command,bytes(u.mem_read(src,words*2))));ret(u)
 main.hook_add(UC_HOOK_CODE,main_code)
 # Fixed kernel reads/writes exactly one frame and preserves the C ABI.
 raw=(project/'disc/C00.CIN').read_bytes()[2048:2048+7680]
 main.mem_write(0x200000,raw)
 area=ms['image'];saved=bytes(main.mem_read(area-4,7688))
 main.mem_write(area-4,b'GUAR'+b'\xa5'*7680+b'TAIL')
 callee={getattr(mc,'UC_M68K_REG_'+n):0x13570000+i for i,n in enumerate(['D2','D3','D4','D5','D6','D7','A2','A3','A4','A5','A6'])}
 for register,value in callee.items():main.reg_write(register,value)
 assert call('main','cinema_copy_checked',(0x200000,area))==sum(struct.unpack('>3840H',raw))&65535
 assert all(main.reg_read(register)==value for register,value in callee.items())
 assert bytes(main.mem_read(area-4,7688))==b'GUAR'+raw+b'TAIL'
 main.mem_write(area-4,saved)
 # This regression executes setup: E000 may be a second A page only if B moves.
 call('main','cinema_initialize')
 assert 0x8405 in regs and 0x8230 in regs,('wrong VDP table selection',regs)
 assert any(len(v)==7680 for _,_,v in dma),'poster not uploaded'
 assert ms['_BSS_ORIGIN']+ms['_BSS_LENGTH']<=0xffd000
 assert ss['_BSS_ORIGIN']+ss['_BSS_LENGTH']<0x1e000
 assert boot['_BSS_ORIGIN']+boot['_BSS_LENGTH']<0x28000
 assert 0x28000+2*63488<=0x80000
 # Publisher executes DMA only in early VBlank and flips only an explicit final packet.
 data=bytes((i*37)&255 for i in range(4096));batch=ms['batch'];payload=batch+8
 main.mem_write(payload,data)
 put(main,batch,2048);put(main,batch+2,0x4000);put(main,batch+4,1);put(main,batch+6,1);put(main,ms['published'],1)
 before=len(dma)
 for line,state in [(223,0),(232,8),(224,0)]:
  vertical[0]=line;blank[0]=state;call('main','vblank_user');assert len(dma)==before and get(main,ms['published'])==1
 vertical[0]=224;blank[0]=8;call('main','vblank_user');assert dma[-1][2]==data and not get(main,ms['published'])
 call('main','cinema_acknowledge',(1,))
 put(main,batch,1792);put(main,batch+2,0x5000);put(main,ms['published'],1);call('main','vblank_user');call('main','cinema_acknowledge',(1,))
 before_regs=len(regs);put(main,batch,0);put(main,batch+4,2);put(main,batch+6,1);put(main,ms['published'],1);call('main','vblank_user')
 assert regs[-1]==0x8238 and len(regs)==before_regs+1
 call('main','cinema_acknowledge',(2,))
 for words,addr,kind,slot in [(2049,0x2000,1,0),(1,0x1ffe,1,0),(2048,0x3800,1,0),(1,0x2000,2,0),(0,0x2000,3,0),(0,0,2,2)]:
  for off,value in [(0,words),(2,addr),(4,kind),(6,slot)]:put(main,batch+off,value)
  put(main,ms['published'],1);before=len(dma);call('main','vblank_user');assert get(main,ms['video_fault']) and len(dma)==before
 put(main,ms['video_fault'],0)
 files={p.name.upper()+';1':p.read_bytes() for p in (project/'disc').glob('*.CIN')}
 active_read=[None];reads_count=[0];tick=[0]
 def cstring(u,a):
  out=bytearray()
  while (b:=get(u,a,1)):out.append(b);a+=1
  return out.decode()
 def sub_code(u,a,size,user):
  if a==ss['stream_file_size']:
   name=cstring(u,get(u,u.reg_read(SP)+4,4));active_read[0]=files.get(name);reads_count[0]+=1;ret(u,len(active_read[0] or b''))
 sub.hook_add(UC_HOOK_CODE,sub_code)
 # Simulated CDC stays busy for twenty service ticks. Current cached frames
 # must remain usable while read-ahead targets the other PRG buffer. This is
 # a scheduling/ownership stress case, not measured physical drive timing.
 waiting=[0]
 def sub_tick():
  tick[0]+=1
  if get(sub,boot['access_op']):
   waiting[0]+=1
   if waiting[0]>=20:
    raw=active_read[0];dest=get(sub,boot['filebuff'],4);assert 0x28000<=dest<0x47000
    sub.mem_write(dest,raw);put(sub,boot['filesize'],len(raw),4);put(sub,boot['access_op_result'],0x64);put(sub,boot['access_op'],0);waiting[0]=0
  call('sub','cinema_sub_tick',(tick[0],))
 # Reset Main after publisher negative probes; run every disc frame through both adapters.
 call('main','ci_init',(ms['cinema'],));put(main,ms['loaded'],0);put(main,ms['link'],0)
 for frame in range(48):
  put(main,0xa12002,1);put(main,0xa12020,0);put(main,0xa12010,0)
  call('main','cinema_request',(frame,));assert get(main,ms['link'])==1
  put(main,0xa12002,2);call('main','cinema_receive',(5,));token=get(main,0xa12010)
  assert token>=2 and get(main,0xa12012)==frame
  put(sub,0xff8002,2);put(sub,0xff8010,token);put(sub,0xff8012,frame);put(sub,0xff8018,1)
  for _ in range(50):
   sub_tick()
   if get(sub,ss['phase'])==2:
    copied=get(sub,ss['copy_at'])
    for _ in range(3):call('sub','cinema_sub_tick',(tick[0],))
    assert get(sub,ss['copy_at'])==copied,'worker exceeded its per-tick copy budget'
   if get(sub,ss['phase'])==4:put(sub,0xff8002,1)
   if get(sub,0xff8020)==token:break
  else:raise AssertionError('Sub did not deliver '+str(frame))
  assert get(sub,0xff8022)==1,(frame,get(sub,0xff8022))
  raw=bytes(sub.mem_read(0x80000,7680));wanted=files[f'C{frame//8:02}.CIN;1'][2048+(frame%8)*7680:2048+(frame%8+1)*7680]
  assert raw==wanted
  main.mem_write(0x200000,raw);put(main,0xa12002,1)
  for off in [0,2,4,10,12,14]:put(main,0xa12020+off,get(sub,0xff8020+off))
  call('main','cinema_receive',(5,));assert get(main,ms['loaded'])==1 and not get(main,0xa12010)
  assert bytes(main.mem_read(ms['image'],7680))==wanted
  put(sub,0xff8010,0);sub_tick();put(main,0xa12020,0);call('main','cinema_receive',(5,));assert get(main,ms['link'])==0
  # Exercise the actual timeline consumer for the two bounded DMA packets.
  assert call('main','ci_upload',(ms['cinema'],4096));assert call('main','ci_upload',(ms['cinema'],3584))
  call('main','ci_tick',(ms['cinema'],30));assert call('main','ci_present',(ms['cinema'],))
  put(main,ms['loaded'],0)
 assert get(main,ms['cinema']+12)==48 and get(main,ms['cinema']+14)==1
 assert reads_count[0]==6,'read-ahead must read each group once'
 # Receiving the next frame can overlap the final immutable upload packet.
 call('main','ci_init',(ms['cinema'],));assert call('main','ci_begin',(ms['cinema'],0))
 assert call('main','ci_upload',(ms['cinema'],4096))
 frame1=files['C00.CIN;1'][2048+7680:2048+15360]
 put(main,ms['loaded'],1);put(main,ms['prefetched'],0);put(main,ms['link'],2);put(main,ms['token'],200);put(main,ms['requested'],1)
 put(main,0xa12002,1);put(main,0xa12020,200);put(main,0xa12022,1);put(main,0xa12024,1);put(main,0xa1202c,200)
 put(main,0xa1202a,sum(struct.unpack('>3840H',frame1))&65535);main.mem_write(0x200000,frame1)
 tail=files['C00.CIN;1'][2048+4096:2048+7680]
 put(main,batch,1792);put(main,batch+2,0x5000);put(main,batch+4,1);put(main,batch+6,1);main.mem_write(batch+8,tail);put(main,ms['published'],1)
 call('main','cinema_receive',(5,))
 assert get(main,ms['prefetched'])==2 and get(main,ms['cinema'])==0
 assert bytes(main.mem_read(ms['image'],7680))==frame1 and bytes(main.mem_read(batch+8,3584))==tail
 vertical[0]=224;blank[0]=8;call('main','vblank_user');call('main','cinema_acknowledge',(1,))
 # The next image is already prefetched. Retiring the mailbox before the
 # current flip must not request that same image again.
 put(main,ms['ack'],0);put(main,0xa12020,0);call('main','cinema_receive',(5,))
 assert not get(main,ms['link'])
 put(main,ms['playing'],1);put(main,ms['stopping'],0)
 call('main','ci_tick',(ms['cinema'],30));call('main','cinema_publish')
 assert not get(main,ms['link']) and get(main,ms['published']) and get(main,batch+4)==2
 call('main','vblank_user')
 # VBlank may have flipped during a foreground copy. The producer must wait
 # for that pending acknowledgement, rather than queue the same flip twice.
 put(main,ms['playing'],1);put(main,ms['stopping'],0)
 assert get(main,ms['ack'])==2 and not get(main,ms['published'])
 call('main','cinema_publish');assert not get(main,ms['published'])
 put(main,ms['ack'],0);call('main','cinema_acknowledge',(2,))
 assert get(main,ms['cinema'])==1 and get(main,ms['cinema']+2)==0 and get(main,ms['loaded'])==1 and not get(main,ms['prefetched'])
 put(main,0xa12020,0);call('main','cinema_receive',(5,));assert not get(main,ms['link'])
 # Real Sub rejects wrong headers, corrupt frame bytes, missing/short files.
 original=files['C00.CIN;1']
 for defect,wanted_result in [('header',4),('pixels',4),('missing',2),('short',4)]:
  raw=bytearray(original)
  if defect=='header':raw[7]^=1
  if defect=='pixels':raw[2048]^=1
  if defect=='missing':files.pop('C00.CIN;1',None)
  else:files['C00.CIN;1']=bytes(raw[:-1] if defect=='short' else raw)
  put(sub,ss['cache_first'],65535);put(sub,ss['cache_first']+2,65535)
  put(sub,ss['io_error'],0);put(sub,ss['phase'],0);put(sub,ss['io'],0)
  put(sub,0xff8020,0);put(sub,0xff8002,2);put(sub,0xff8010,100);put(sub,0xff8012,0);put(sub,0xff8018,0)
  for _ in range(30):
   sub_tick()
   if get(sub,ss['phase'])==4:put(sub,0xff8002,1)
   if get(sub,0xff8020)==100:break
  else:raise AssertionError('Bad input did not retire '+defect)
  assert get(sub,0xff8022)==wanted_result,(defect,get(sub,0xff8022))
  put(sub,0xff8010,0);sub_tick();files['C00.CIN;1']=original
 # Cancellation does not reclaim a live reader; Main retires only the matching reply.
 put(main,ms['stopping'],0);put(main,0xa12002,1);call('main','cinema_request',(0,));put(main,0xa12002,2);call('main','cinema_receive',(5,))
 call('main','cinema_receive',(9000,));assert get(main,ms['stopping']) and get(main,ms['failed'])==6
 assert get(main,0xa12010)!=0,'timeout stole an outstanding mailbox'
 return True

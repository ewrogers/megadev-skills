"""Run the level parser/model/cache and publication on original M68000 instructions."""
import struct

def check_levels(u,sym):
 from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
 from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
 stop=0x300000;stack=0xFFDF00;model=0x210000
 u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
 def call(name,args=()):
  u.reg_write(UC_M68K_REG_A7,stack);u.mem_write(stack,struct.pack('>I',stop)+b''.join(struct.pack('>I',n&0xffffffff) for n in args))
  u.emu_start(sym[name],stop,count=2000000)
  assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not finish'
  assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
  return u.reg_read(UC_M68K_REG_D0)&65535
 def word(a):return int.from_bytes(u.mem_read(a,2),'big')
 def put(a,n):u.mem_write(a,(n&65535).to_bytes(2,'big'))
 world=sym['world'];game=sym['game'];size=sym['res_world_end']-sym['res_world']
 assert call('lv_open',(world,sym['res_world'],size))==1 and word(world+56)==2
 assert call('lv_open',(model,sym['res_world'],size-1))==0 and not word(model+56)
 call('lv_init',(world,game))
 for i in range(60):call('lv_advance',(world,game,5,8,0))
 expected=bytes(u.mem_read(game,34));assert word(game+2)==200 and word(game+6)==40 and word(game+14)==60
 call('lv_init',(world,game))
 for i in range(50):call('lv_advance',(world,game,6,8,0))
 assert bytes(u.mem_read(game,34))==expected,'PAL/NTSC mismatch'
 call('lv_advance',(world,game,5,0,128));frozen=bytes(u.mem_read(game,34))
 for i in range(50):call('lv_advance',(world,game,6,8,0))
 assert bytes(u.mem_read(game,34))==frozen
 call('lv_init',(world,game));assert call('lv_advance',(world,game,65535,8,0))==4 and word(game+32)==1
 put(game+2,736);put(game+4,264);call('lv_advance',(world,game,5,0,64))
 assert word(game)==1 and word(game+2)==80 and word(game+28)==1
 put(game+2,40);put(game+4,264);call('lv_advance',(world,game,5,0,64))
 assert word(game)==0 and word(game+2)==680 and word(game+28)==2
 # Actual compiled quadrant flags, empty foreground and both world palettes.
 assert call('lp_word',(world,sym['res_attrs'],1,0,0,0))==0,'Empty world tile must be transparent, not the opaque font space'
 assert call('lp_word',(world,sym['res_attrs'],0,0,0,0))==0x4110
 assert call('lp_word',(world+14,sym['res_attrs'],0,0,0,0))==0x6150
 area_data=int.from_bytes(u.mem_read(world,4),'big');original=word(area_data+20)
 put(area_data+20,original|0xC000)
 assert call('lp_word',(world,sym['res_attrs'],0,0,0,0))==0x5913,'Target flipped quadrants disagree'
 put(area_data+20,original)
 # A valid-size bundle with a broken exit reference must still fail closed.
 copy=model+4096;raw=bytes(u.mem_read(sym['res_world'],size));u.mem_write(copy,raw)
 first=int.from_bytes(raw[12:16],'big');exit_offset=first+20+48*32*5+5*8
 put(copy+exit_offset+10,4)
 assert call('lv_open',(model,copy,size))==0 and not word(model+56)
 p=model+256
 assert call('lv_patch',(p,world,sym['res_attrs'],248,248,256,256,0))==1
 assert word(p+416)==4 and word(p+418)==192
 for i in range(4):
  address,count,step,offset=struct.unpack('>4H',u.mem_read(p+i*8,8))
  assert count in (32,64) and step in (2,128) and offset+count<=192
 if 'level_tour' in sym:
  call('lv_init',(world,game))
  for i in range(7):call('level_tour')
  assert word(sym['tour_ok'])==1 and word(game)==1
 call('lv_init',(world,game));put(sym['hz'],60);put(sym['cached_area'],0);put(sym['cached_debug'],0);put(sym['reload_pending'],0)
 put(sym['committed_x'],0);put(sym['committed_y'],168)
 # Publish a diagonal cache crossing with both plane rows and columns.
 put(game+6,8);put(game+8,176);call('level_publish')
 pending=word(sym['published']);frame=sym['frames']+(pending-1)*2064;count=word(frame+1312)
 assert pending in (1,2) and 1<=count<=8 and word(frame+2060)==4 and word(frame+2062)==192
 snapshot=bytes(u.mem_read(frame,2064));call('level_publish');assert bytes(u.mem_read(frame,2064))==snapshot
 writes=[];early=True
 def read(cpu,access,address,size,value,user):
  if address==0xC00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
  if address==0xC00008:cpu.mem_write(address,(0xE000 if early else 0xE900).to_bytes(size,'big'))
 def write(cpu,access,address,size,value,user):
  if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
 hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
 call('level_configure');assert (0xC00004,2,0x9001) in writes,'Ring model requires explicit H64/V32 VDP geometry'
 assert (0xC00004,2,0x8F02) in writes
 writes.clear()
 early=False;call('vblank_user');assert not writes and word(sym['published'])==pending
 early=True;call('vblank_user');assert not word(sym['published']) and word(sym['submitted'])==1
 assert word(sym['committed_x'])==8 and word(sym['committed_y'])==176
 actual=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
 expected=bytearray()
 for i in range(4):
  address,n,step,offset=struct.unpack('>4H',u.mem_read(frame+1644+i*8,8));expected+=u.mem_read(frame+1676+offset*2,n*2)
 expected+=u.mem_read(frame,count*8);expected+=struct.pack('>4H',1016,1016,144,144);expected+=u.mem_read(frame+1320,320)
 assert actual==expected and len(actual)<=776 and word(sym['upload_high'])==len(actual)
 controls=[(n,v) for a,n,v in writes if a==0xC00004]
 assert not any(n==4 and v==0xC0000000 for n,v in controls),'Normal frame touched CRAM'
 assert [v for n,v in controls if n==2]==[0x8F80,0x8F02,0x8F80,0x8F02,0x8F02]
 writes.clear();call('vblank_user');assert not writes
 # Corrupt run metadata fails before any VDP transfer or camera commitment.
 put(frame+2060,5);put(sym['published'],pending);call('vblank_user');assert not writes and word(sym['video_fault'])==1 and not word(sym['published'])
 # Production walking cannot install a map or blank the display in foreground.
 put(sym['video_fault'],0);call('lv_init',(world,game));put(sym['cached_area'],0);put(sym['cached_debug'],0);put(sym['reload_pending'],0)
 for i in range(24):
  assert call('level_update',(1,8,0))==0 and not writes
  put(sym['published'],0)
 u.hook_del(hr);u.hook_del(hw)
 return True

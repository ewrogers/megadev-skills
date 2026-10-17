"""Execute compiled model and VBlank publisher; MMIO capture is not a VDP model."""
def check_graphics(u,sym):
    from unicorn import UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
    stop=0x300000;stack=0xFFDF00;model=0x210000
    u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
    def call(name,args=()):
        u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((a&0xFFFFFFFF).to_bytes(4,'big') for a in args))
        u.emu_start(sym[name],stop,count=1000000)
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not finish'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
    def word(address):return int.from_bytes(u.mem_read(address,2),'big')
    def put(address,value):u.mem_write(address,value.to_bytes(2,'big'))
    call('gfx_begin',(model,80))
    for i in range(20):call('gfx_sprite',(model,0,100,8,8,288,1,0,0))
    assert word(model+1312)==20
    before=bytes(u.mem_read(model,1316))
    call('gfx_sprite',(model,0,100,8,8,288,1,0,0))
    assert bytes(u.mem_read(model,1316))==before,'Rejected candidate changed accepted SAT/budgets'
    assert word(model+1318)==1
    call('gfx_begin',(model,80));call('gfx_sprite',(model,-31,-23,32,24,288,1,0,1))
    assert bytes(u.mem_read(model,8))==bytes.fromhex('00690e0029200061')
    state=0x211000
    call('gfx_reset',(state,))
    for i in range(50):call('gfx_step',(state,8,50))
    pal_state=bytes(u.mem_read(state,20));assert word(state)==280 and word(state+6)==60
    call('gfx_reset',(state,))
    for i in range(60):call('gfx_step',(state,8,60))
    assert bytes(u.mem_read(state,20))==pal_state,'PAL/NTSC elapsed-second behavior differs'
    base=0x212000;output=base+128
    u.mem_write(base,b''.join((i*2&0xEEE).to_bytes(2,'big') for i in range(64)))
    call('gfx_palette',(base,output,0,3))
    assert bytes(u.mem_read(output,32))==bytes(u.mem_read(base,32))
    assert bytes(u.mem_read(output+32,96))==b'\x0E\xEE'*48
    call('gfx_shade',(0,32767));assert u.reg_read(UC_M68K_REG_D0)&0xFFFF==0xEEE
    for tick,expected in [(5,0),(6,1),(12,0)]:
        call('gfx_frame',(tick,6,2));assert u.reg_read(UC_M68K_REG_D0)&0xFFFF==expected
    # Frame layout is asserted by the host C test. Each target uses 16-bit words.
    frame=sym['frames'];u.mem_write(frame,bytes(1612*2))
    call('gfx_begin',(frame,24))
    for i in range(24):call('gfx_sprite',(frame,(i%8)*32,(i//8)*32+40,32,24,288,1,0,i&1))
    assert word(frame+1312)==24
    palette=b''.join(((i*2)&0xEEE).to_bytes(2,'big') for i in range(64))
    scroll=bytes.fromhex('03ff03f0');hud=b''.join((0x8020+i%60).to_bytes(2,'big') for i in range(80))
    u.mem_write(frame+1320,palette+scroll+hud)
    writes=[]
    def capture(cpu,access,address,size,value,user):
        if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
    hook=u.hook_add(UC_HOOK_MEM_WRITE,capture)
    put(sym['published'],1);call('vblank_user')
    assert word(sym['published'])==0 and word(sym['submitted'])==1
    assert word(sym['upload_high'])==484 and not word(sym['upload_fault'])
    actual=b''.join(value.to_bytes(size,'big') for address,size,value in writes if address==0xC00000)
    expected=bytes(u.mem_read(frame,24*8))+scroll+palette+hud
    assert actual==expected,'Frame transfer words/order differ from published buffer'
    assert bytes(u.mem_read(0xFFFB80,128))==palette,'BIOS palette cache was not kept consistent'
    controls=[value for address,size,value in writes if address==0xC00004]
    assert len(controls)==5,'Expected SAT, scroll, CRAM and two HUD rows'
    writes.clear();call('vblank_user');assert not writes,'Idle callback uploaded stale frame'
    put(sym['published'],2);call('vblank_user')
    actual=b''.join(value.to_bytes(size,'big') for address,size,value in writes if address==0xC00000)
    assert actual==bytes(300),'Second empty buffer did not replace the previous SAT'
    writes.clear()
    put(frame+1312,25);put(sym['published'],1);call('vblank_user')
    assert not writes and word(sym['upload_fault'])==1 and not word(sym['published'])
    u.hook_del(hook)
    return True

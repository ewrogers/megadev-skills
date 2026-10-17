"""Execute the actual protocol, platform and IRQ code with scripted MMIO/BIOS."""
def check_input(u,sym,ea=False):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_HOOK_CODE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR
    import unicorn.m68k_const as regs
    stop=0x300000;stack=0xFFDF00;data=0x210000
    u.mem_map(stop,0x1000);u.mem_map(0xA10000,0x2000);u.mem_map(0xC00000,0x1000)
    def word(a):return int.from_bytes(u.mem_read(a,2),'big')
    def put(a,value):u.mem_write(a,value.to_bytes(2,'big'))
    def call(name,args=(),sr=0x2700):
        u.reg_write(UC_M68K_REG_SR,sr);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((x&0xFFFFFFFF).to_bytes(4,'big') for x in args))
        u.emu_start(sym[name],stop,count=2000000)
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
    # Normal AUTO builds exercise real read/write callbacks to both data ports.
    pad=[63,51,63,51,63,48,59,63]
    ea_pads=[pad,[63,35,63,35,63,35,63,35],[63,51,63,51,63,48,62,63],[63]*8]
    queue=({0xA10003:[3,0]+[ea_pads[player][phase] for phase in range(8) for player in range(4)]} if ea else {0xA10003:pad.copy(),0xA10005:[0,27,27,15,31,0,17,0,18,0,19]})
    io_writes=[];bus_reads=0
    def read(cpu,access,address,size,value,user):
        nonlocal bus_reads
        if address in queue:
            assert queue[address],'Unexpected extra input sample'
            u.mem_write(address,bytes([queue[address].pop(0)]))
        if address==0xA11100:bus_reads+=1
    def write(cpu,access,address,size,value,user):
        if address in [0xA10003,0xA10005,0xA10009,0xA1000B]:io_writes.append((address,value&255))
    rh=u.hook_add(UC_HOOK_MEM_READ,read);wh=u.hook_add(UC_HOOK_MEM_WRITE,write)
    u.mem_write(0xA11100,b'\0\0');call('input_poll',(data,),sr=0x2000)
    assert u.reg_read(UC_M68K_REG_SR)&0x700==0,'Input changed the caller interrupt mask'
    assert not any(queue.values())
    assert word(data)==2 and word(data+4)==256,'Six-button X was not decoded'
    if ea:
        assert word(data+10)==1 and word(data+14)==64
        assert word(data+20)==2 and word(data+24)==1024
        assert not word(data+30)
        assert all(not word(data+i*10) for i in range(4,8))
        assert io_writes[-1]==(0xA10005,124)
    else:
        assert word(data+40)==3 and word(data+44)==64,'Port 2 mouse/button mapping failed'
        assert word(data+46)==2 and word(data+48)==65533,'Mouse axes not right/down positive'
        assert io_writes[-1]==(0xA10005,96),'Mouse not left idle'
        team=[0,0,0,1,2,15,14,11,15,15,11,0,1,0,2,0,3]
        queue.update({0xA10003:[3,15]+[n|(16 if i&1 else 0) for i,n in enumerate(team)],0xA10005:[63,63]})
        call('input_poll',(data,));assert not any(queue.values())
        assert word(data+4)==65 and word(data+14)==256 and word(data+20)==3
        queue.update({0xA10003:[0,27]+[0]*128,0xA10005:[63,63]})
        call('input_poll',(data,));assert not any(queue.values())
        assert all(word(data+i*10+2)==2 and not word(data+i*10+4) for i in range(4))
    assert bytes(u.mem_read(0xA11100,2))==b'\0\0','Previously granted Z80 bus was released'
    # A request that never grants must fail closed, not hang or touch I/O.
    io_writes.clear();queue.clear();bus_reads=0;u.mem_write(0xA11100,b'\x01\0')
    call('input_poll',(data,));assert not io_writes and bus_reads<=515
    assert u.reg_read(UC_M68K_REG_SR)&0x700==0x700,'Bus failure changed the caller mask'
    assert word(sym['input_bus_fault'])==1 and bytes(u.mem_read(0xA11100,2))==b'\0\0'
    for i in range(8):assert word(data+i*10+2)==5 and not word(data+i*10+4)
    u.hook_del(rh);u.hook_del(wh)
    # New controller/type samples establish a baseline rather than a false edge.
    slot=data+128;sample=data+160
    call('in_reset',(slot,));u.mem_write(sample,bytes.fromhex('00020000008000000000'))
    call('in_update',(slot,sample,60));assert word(slot)==128 and not word(slot+2)
    u.mem_write(sample,bytes.fromhex('00020000000000000000'));call('in_update',(slot,sample,60));assert word(slot+4)==128
    # Execute the real VBlank frame commit; captured ports are not a VDP model.
    frame=sym['frames'];u.mem_write(frame,bytes(452*2))
    u.mem_write(frame,bytes.fromhex('002a804100538042'));put(frame+384,2);put(frame+450,0)
    put(sym['published'],1);writes=[]
    def vdp(cpu,access,address,size,value,user):
        if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
    vh=u.hook_add(UC_HOOK_MEM_WRITE,vdp)
    call('vblank_user')
    assert word(sym['published'])==0 and word(sym['submitted'])==1
    assert word(sym['displayed']+42*2)==0x8041 and word(sym['displayed']+83*2)==0x8042
    actual=b''.join(value.to_bytes(size,'big') for address,size,value in writes if address==0xC00000)
    assert actual==bytes.fromhex('80418042')+bytes(8)
    writes.clear();put(frame,1120);put(sym['published'],1);call('vblank_user')
    assert not writes and word(sym['video_fault'])==1 and not word(sym['published'])
    u.hook_del(vh)
    # Mock ONLY the documented BIOS INT2 pulse, then execute MOVEM through the RTE boundary.
    u.mem_write(0x360,b'\x4E\x75');pulses=[]
    def pulse(cpu,address,size,user):
        if address==0x360:pulses.append(address)
    h=u.hook_add(UC_HOOK_CODE,pulse)
    saved={getattr(regs,'UC_M68K_REG_'+name):0x12340000+i for i,name in enumerate([*[f'D{i}' for i in range(8)],*[f'A{i}' for i in range(7)]])}
    for register,value in saved.items():u.reg_write(register,value)
    u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
    u.mem_write(stack,b'\x20\0'+stop.to_bytes(4,'big'))
    # Unicorn's M68000 backend raises on RTE. Execute through register restore,
    # stop at that boundary and verify the real return opcode/exception frame.
    end=sym['input_irq_return'];u.emu_start(sym['input_irq'],end,count=20000)
    assert u.reg_read(UC_M68K_REG_PC)==end and u.reg_read(UC_M68K_REG_A7)==stack
    assert bytes(u.mem_read(end,2))==b'\x4E\x73'
    assert bytes(u.mem_read(stack,6))==b'\x20\0'+stop.to_bytes(4,'big')
    assert pulses==[0x360] and all(u.reg_read(reg)==value for reg,value in saved.items())
    u.hook_del(h)
    return True

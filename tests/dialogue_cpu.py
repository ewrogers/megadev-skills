"""Execute compiled dialogue policy and row publication on the original MC68000."""
def check_dialogue(u,sym):
    from unicorn import UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
    stop=0x300000;stack=0xFFDF00
    u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
    m,c,p,b=(sym[n] for n in ('dialogue','desired','painter','batch'))
    def call(name,args=()):
        u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((a&0xFFFFFFFF).to_bytes(4,'big') for a in args))
        u.emu_start(sym[name],stop,count=1500000)
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' failed to return on MC68000'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' broke C ABI'
        return u.reg_read(UC_M68K_REG_D0)&0xFFFF
    def word(a):return int.from_bytes(u.mem_read(a,2),'big')
    def put(a,v):u.mem_write(a,v.to_bytes(2,'big'))
    def present():call('d_present',(m,word(m+1080)))
    call('d_init',(m,));put(m+1080,65534);put(sym['published'],1)
    assert call('dialogue_controls',(240,))==32,'Generation reused while a renderer request was pending'
    put(sym['published'],0);assert call('dialogue_controls',(240,))==240
    call('d_init',(m,));assert call('d_start',(m,0))
    call('d_step',(m,64,0,0));assert not word(m+1074) and not call('d_voice',(m,))
    present();assert call('d_voice',(m,))==1
    for _ in range(60):call('d_step',(m,0,0,5))
    ntsc=bytes(u.mem_read(m,1102))
    call('d_init',(m,));call('d_start',(m,0));present()
    for _ in range(50):call('d_step',(m,0,0,6))
    assert bytes(u.mem_read(m,1102))==ntsc,'Equal-time PAL/NTSC dialogue differs'
    for story in (0,1):
        call('d_init',(m,));call('d_start',(m,story));seen=0
        for _ in range(40):
            if word(m+1066)!=1:break
            seen|=1<<word(m+1068);present();call('d_step',(m,64,0,0))
            assert call('d_draw',(c,m,50 if story else 60))
            assert all(word(c+i*2)&2047<0x27C for i in range(1120))
            call('d_step',(m,64,0,0))
        assert word(m+1066)==2 and seen==(0xE0 if story else 0x17)
    call('d_init',(m,));call('d_start',(m,0));present()
    for _ in range(2):call('d_step',(m,64,0,0))
    present();call('d_step',(m,64,0,0));call('d_step',(m,2,0,0));call('d_step',(m,64,0,0))
    assert word(m+1068)==3 and word(m+1084)==2,'Alternate choice selected wrong branch'
    before=bytes(u.mem_read(c,2240));put(m+1066,8)
    assert not call('d_draw',(c,m,60)) and bytes(u.mem_read(c,2240))==before
    call('d_init',(m,));assert call('d_draw',(c,m,60));call('ui_painter_init',(p,))
    writes=[]
    def capture(cpu,access,address,size,value,user):
        if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
    hook=u.hook_add(UC_HOOK_MEM_WRITE,capture)
    def irq(status=8,line=224):
        put(0xC00004,status);put(0xC00008,line<<8);writes.clear();call('vblank_user')
    for step in range(7):
        assert call('ui_prepare',(p,c,65534,b))
        expected=bytes(u.mem_read(b+16,320));put(sym['published'],1)
        if step==0:
            for status,line in ((0,224),(8,223),(8,231)):
                irq(status,line);assert not writes and word(sym['published'])==1
        irq();assert not word(sym['published'])
        actual=b''.join(v.to_bytes(size,'big') for a,size,v in writes if a==0xC00000)
        assert actual==expected
        controls=[v for a,size,v in writes if a==0xC00004 and size==2]
        assert controls==([0x8238] if step==6 else []),'Premature page switch'
        assert call('ui_ack',(p,b))
    assert word(0xFFFDB8)==0x8238
    if 'dialogue_tour' in sym:
        call('d_init',(m,));call('ui_painter_init',(p,));seen=0
        for _ in range(240):
            call('dialogue_tour',(150,))
            if word(m+1066)==1:seen|=1<<word(m+1068)
            assert call('d_draw',(c,m,60))
            if call('ui_prepare',(p,c,word(m+1080),b)):
                put(sym['published'],1);irq();assert call('ui_ack',(p,b))
                if not word(p+4488):call('d_present',(m,word(p+4480)))
            if seen==0xF7 and word(m+1066)==0:break
        assert seen==0xF7 and word(m+1066)==0,'Tour did not complete both stories'
    u.hook_del(hook)
    return True

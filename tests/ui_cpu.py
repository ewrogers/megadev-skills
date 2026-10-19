"""Execute original-68000 UI and publication code; MMIO capture is not a VDP."""
def check_ui(u,sym):
    from unicorn import UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
    stop=0x300000;stack=0xFFDF00;model=0x210000;canvas=model+256;painter=canvas+2240;batch=painter+4496
    u.mem_map(stop,0x1000);u.mem_map(0xC00000,0x1000)
    def call(name,args=()):
        u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((a&0xFFFFFFFF).to_bytes(4,'big') for a in args))
        u.emu_start(sym[name],stop,count=1000000)
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' failed to return on MC68000'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' broke C ABI'
        return u.reg_read(UC_M68K_REG_D0)&0xFFFF
    def word(a):return int.from_bytes(u.mem_read(a,2),'big')
    def put(a,v):u.mem_write(a,v.to_bytes(2,'big'))
    call('ui_init',(model,));call('ui_input',(model,2,0,0));call('ui_input',(model,64,0,0))
    assert word(model)==2
    call('ui_input',(model,8,0,0));call('ui_input',(model,2,0,0));assert word(model+6)==4
    call('ui_input',(model,64,0,0));assert word(model)==7
    call('ui_input',(model,16,0,0));assert word(model)==2 and word(model+6)==4
    call('ui_init',(model,));call('ui_input',(model,2,0,0))
    for _ in range(60):call('ui_input',(model,0,2,5))
    ntsc=bytes(u.mem_read(model,50))
    call('ui_init',(model,));call('ui_input',(model,2,0,0))
    for _ in range(50):call('ui_input',(model,0,2,6))
    assert bytes(u.mem_read(model,50))==ntsc,'PAL/NTSC UI repeat diverges'
    for face in (0,1):
        put(model+28,face)
        for page in range(8):
            put(model,page);assert call('ui_draw',(canvas,model,60))
            assert all(word(canvas+i*2)&2047<0x1C8 for i in range(1120))
    put(model+2,4);before=bytes(u.mem_read(canvas,2240))
    assert not call('ui_draw',(canvas,model,60)) and bytes(u.mem_read(canvas,2240))==before
    call('ui_init',(model,));call('ui_draw',(canvas,model,60));call('ui_painter_init',(painter,))
    writes=[]
    def capture(cpu,access,address,size,value,user):
        if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
    hook=u.hook_add(UC_HOOK_MEM_WRITE,capture)
    def irq(status=8,line=224):
        put(0xC00004,status);put(0xC00008,line<<8);writes.clear();call('vblank_user')
    for step in range(7):
        assert call('ui_prepare',(painter,canvas,0,batch))
        expected=bytes(u.mem_read(batch+16,320))
        u.mem_write(sym['batch'],bytes(u.mem_read(batch,336)));put(sym['published'],1)
        if step==0:
            for status,line in ((0,224),(8,223),(8,231)):
                irq(status,line);assert not writes and word(sym['published'])==1,'Late/active callback consumed or wrote UI'
            assert not call('ui_prepare',(painter,canvas,2,batch)),'Reused in-flight publication'
        irq()
        assert not word(sym['published']) and word(sym['consumed'])==word(batch+6)
        actual=b''.join(v.to_bytes(size,'big') for a,size,v in writes if a==0xC00000)
        assert actual==expected,'UI row transfer differs from published immutable words'
        controls=[(size,v) for a,size,v in writes if a==0xC00004]
        addresses=[0xE000+(step*4+i)*128 for i in range(4)]
        assert [v for size,v in controls if size==4]==[((a&0x3FFF)<<16)|(a>>14)|0x40000000 for a in addresses]
        assert [v for size,v in controls if size==2]==([0x8238] if step==6 else []),'Page flipped before all rows arrived'
        assert call('ui_ack',(painter,batch))
    assert word(0xFFFDB4+4)==0x8238,'BIOS register cache differs after page flip'
    irq();assert not writes,'Idle callback repeated stale words'
    put(sym['batch'],5);put(sym['published'],1);irq()
    assert not writes and word(sym['video_fault'])==1 and word(sym['published'])==1,'Malformed batch did not fail closed'
    if 'ui_tour' in sym:
        # Execute the real compiled tour and its page publication path. Merely
        # building a tour must not count as proving that it reaches every page.
        gm,gc,gp,gb=(sym[n] for n in ('model','desired','painter','batch'))
        put(sym['published'],0);put(sym['video_fault'],0);put(sym['committed_pages'],0)
        call('ui_init',(gm,));call('ui_painter_init',(gp,))
        for _ in range(240):
            call('ui_tour',(150,))
            assert call('ui_draw',(gc,gm,60))
            key=call('ui_key',(gm,))
            if call('ui_prepare',(gp,gc,key,gb)):
                put(sym['published'],1);irq();assert not word(sym['published'])
                assert call('ui_ack',(gp,gb))
                if not word(gp+4488):
                    put(sym['committed_pages'],word(sym['committed_pages'])|(1<<(word(gp+4480)%8)))
            if word(gm+48):break
        assert word(sym['committed_pages'])==255 and word(gm+48)==1,'Tour failed to commit all eight screens'
    u.hook_del(hook)
    return True

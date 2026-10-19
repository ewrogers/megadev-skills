"""Execute integration adapters with shared registers and a named-file BIOS model.

No drive, interrupt latency or analog/timing accuracy is claimed.
"""
from startup import check,symbols

def check_glass_save(project,hz):
    from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_A0,UC_M68K_REG_A1
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    mailbox=bytearray(0x30);mailbox[2:4]=b'\0\1';files={};media=3;write_failure=False;calls=[]
    names=[b'MDGLASS__A_',b'MDGLASS__B_']
    for kind,(u,sy) in contexts.items():
        u.mem_map(0x300000,0x1000);base=0xa12000 if kind=='main' else 0xff8000
        def read(cpu,access,address,size,value,user,base=base):
            if base<=address<base+0x30:cpu.mem_write(address,bytes(mailbox[address-base:address-base+size]))
        def write(cpu,access,address,size,value,user,base=base,kind=kind):
            if base<=address<base+0x30:
                off=address-base
                assert (kind=='main' and 0x10<=off<0x20) or (kind=='sub' and 0x20<=off and off!=0x26),'Wrong register writer'
                mailbox[off:off+size]=(value&((1<<(size*8))-1)).to_bytes(size,'big')
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    def get(u,p,n=2):return int.from_bytes(u.mem_read(p,n),'big')
    def put(u,p,v,n=2):u.mem_write(p,v.to_bytes(n,'big'))
    def bios(u,address,size,user):
        op=u.reg_read(UC_M68K_REG_D0)&65535;a0=u.reg_read(UC_M68K_REG_A0);a1=u.reg_read(UC_M68K_REG_A1);carry=False
        calls.append(op);assert op in (0,2,3,4),'Integration exposed format/delete/unknown BIOS operation'
        if op==0:
            assert ss['_BSS_ORIGIN']<=a0 and a0+0x640<=ss['_BSS_ORIGIN']+ss['_BSS_LENGTH']
            u.reg_write(UC_M68K_REG_D1,0 if media==3 else media);carry=media!=3
        else:
            name=bytes(u.mem_read(a0,11));assert name in names,'Integration touched another game save'
            if op==2:
                carry=name not in files;u.reg_write(UC_M68K_REG_D0,1);u.reg_write(UC_M68K_REG_D1,0)
                u.reg_write(UC_M68K_REG_A0,0xfe0001)
            elif op==3:
                carry=name not in files
                if not carry:u.mem_write(a1,files[name]);u.reg_write(UC_M68K_REG_D0,1);u.reg_write(UC_M68K_REG_D1,0)
            else:
                assert get(u,a0+11,1)==0 and get(u,a0+12)==1 and u.reg_read(UC_M68K_REG_D1)==0
                carry=write_failure
                if not carry:files[name]=bytes(u.mem_read(a1,64))
        sr=u.reg_read(UC_M68K_REG_SR);u.reg_write(UC_M68K_REG_SR,(sr|1) if carry else sr&~1)
        stack=u.reg_read(UC_M68K_REG_A7);u.reg_write(UC_M68K_REG_PC,get(u,stack,4));u.reg_write(UC_M68K_REG_A7,stack+4)
    sub.hook_add(UC_HOOK_CODE,bios,begin=0x5f16,end=0x5f16)
    def call(kind,name,*args):
        u,sy=contexts[kind];stack=0xffdf00 if kind=='main' else 0x1ff00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((a&0xffffffff).to_bytes(4,'big') for a in args))
        u.emu_start(sy[name],stop,count=2000000)
        assert u.reg_read(UC_M68K_REG_PC)==stop and u.reg_read(UC_M68K_REG_A7)==stack+4,name+' damaged execution/stack'
        return u.reg_read(UC_M68K_REG_D0)
    def m(name):return get(main,ms[name])
    game=0xffdc00
    def game_set(values):main.mem_write(game,b''.join(n.to_bytes(2,'big') for n in values))
    step=300//hz
    def begin(op,values=(1,48,(128<<4)|1)):
        game_set(values);assert call('main','checkpoint_request',op,game)==1
        before=bytes(mailbox);assert not call('main','checkpoint_request',op,game) and bytes(mailbox)==before
        assert not call('main','stream_start',0),'Reader stole checkpoint mailbox'
    def retire(frontend=False):
        call('sub','checkpoint_sub_tick');call('main','checkpoint_service' if frontend else 'checkpoint_main_tick',step)
        assert m('checkpoint_pending')==2
        assert not call('main','stream_start',0),'Reader reused live checkpoint reply'
        call('sub','checkpoint_sub_tick');call('main','checkpoint_service' if frontend else 'checkpoint_main_tick',step)
        assert not m('checkpoint_pending');return m('checkpoint_result')
    begin(1);assert retire()==4 and not files
    begin(2);game_set((2,200,(128<<4)|7));assert retire()==1
    assert list(files)==[names[0]]
    begin(1);assert retire()==2 and m('checkpoint_reply')==1 and get(main,ms['checkpoint_reply']+4)==(128<<4)|1
    begin(2,(2,64,(128<<4)|7));assert retire()==1
    raw=bytearray(files[names[1]]);raw[60]^=1;files[names[1]]=bytes(raw)
    begin(1);assert retire()==3 and m('checkpoint_reply')==1
    write_failure=True;begin(2);assert retire()==6;write_failure=False
    media=1;before=len(calls);begin(2);assert retire()==5 and calls[before:]==[0];media=3
    # No semantic load/save of impossible coordinates or out-of-order objectives.
    for values in [(1,120,(80<<4)|1),(2,64,(128<<4)|4),(2,64,(128<<4)|8),(9,48,(128<<4)|1)]:
        game_set(values);before=bytes(mailbox);assert not call('main','checkpoint_request',2,game) and bytes(mailbox)==before
    begin(2);before=len(calls);put(sub,ss['audio_state']+10,1)
    call('sub','checkpoint_sub_tick');assert len(calls)==before
    put(sub,ss['audio_state']+10,0);put(sub,ss['access_op'],1,1)
    call('sub','checkpoint_sub_tick');assert len(calls)==before
    put(sub,ss['access_op'],0,1);assert retire()==1
    flow=ms['flow'];g=ms['garden'];field=lambda i:get(main,flow+i*2)
    def gp(offset,value):put(main,g+1066+offset*2,value)
    def gg(offset):return get(main,g+1066+offset*2)
    call('main','scene_init',flow);put(main,flow,1);put(main,flow+2,1)
    call('main','garden_init',g);gp(1,103);gp(2,128);gp(3,1);gp(4,3)
    put(main,ms['stream_phase'],2);assert not call('main','checkpoint_begin',2) and not field(9)
    put(main,ms['stream_phase'],0);assert call('main','checkpoint_begin',2) and field(9)==1
    gp(1,205);call('main','garden_goto',0);call('main','scene_service',120);assert field(3)==2
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step);call('main','scene_service',0)
    assert field(9)==1 and field(3)==2
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step);assert not field(9)
    # Continue is explicitly revoked when the player opens another title page.
    call('main','scene_init',flow);call('main','garden_init',g);gp(5,1)
    assert call('main','checkpoint_begin',1);gp(4,5)
    retire(True);assert not m('restore_pending') and field(3)==0
    call('main','scene_init',flow);call('main','garden_init',g);gp(5,1)
    assert call('main','checkpoint_begin',1);retire(True)
    assert field(1)==1 and m('restore_pending')
    put(main,flow+6,4);put(main,ms['active'],0);put(main,ms['transition'],0);put(main,ms['published'],0)
    put(main,ms['painter']+4480,1);call('main','scene_service',0)
    assert field(0)==1 and gg(1)==103 and gg(2)==128 and gg(3)==1 and not m('restore_pending')
    # A timeout quarantines the save slot AND its owning scene lease.
    put(main,flow+6,0);gp(4,3);assert call('main','checkpoint_begin',2)
    call('main','checkpoint_service',9000)
    assert m('checkpoint_pending')==3 and m('checkpoint_fault')==1 and field(9)==1
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    assert m('checkpoint_pending')==3 and field(9)==1 and not call('main','checkpoint_begin',2)
    assert all(op not in (1,5,6) for op in calls)
    return True

def check_glass_model(u,sym):
    import struct
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    u.mem_map(0x300000,0x1000);u.mem_map(0xC00000,0x1000)
    def call(name,*args):
        stack=0xffdf00;u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,struct.pack('>I',0x300000)+b''.join(struct.pack('>I',n&0xffffffff) for n in args))
        u.emu_start(sym[name],0x300000,count=3000000)
        assert u.reg_read(UC_M68K_REG_PC)==0x300000 and u.reg_read(UC_M68K_REG_A7)==stack+4,name
        return u.reg_read(UC_M68K_REG_D0)&65535
    def get(p):return int.from_bytes(u.mem_read(p,2),'big')
    def put(p,n):u.mem_write(p,(n&65535).to_bytes(2,'big'))
    g=sym['garden'];gp=lambda i,n:put(g+1066+i*2,n);gg=lambda i:get(g+1066+i*2)
    call('garden_init',g);assert gg(4)==4 and not call('garden_step',g,5,0,64)
    gp(15,1);assert call('garden_step',g,5,0,64)==1
    call('garden_enter',g,0);gp(1,48);gp(15,1)
    for _ in range(60):call('garden_step',g,5,8,0)
    expected=bytes(u.mem_read(g,1104));assert gg(1)==168 and gg(11)==60
    call('garden_enter',g,0);gp(1,48);gp(15,1);gp(11,0)
    for _ in range(50):call('garden_step',g,6,8,0)
    # Irrelevant request flags/face match too; same authoritative motion clock.
    assert bytes(u.mem_read(g,1104))==expected
    call('garden_step',g,5,0,128);gp(15,1);frozen=(gg(1),gg(2),gg(11))
    for _ in range(30):call('garden_step',g,6,8,0)
    assert (gg(1),gg(2),gg(11))==frozen and gg(4)==3
    gp(5,1);assert call('garden_step',g,5,0,64)==2
    def finish(node,choice=0):
        assert call('garden_open',g,node)
        for _ in range(12):
            if gg(4)!=1:break
            gp(15,1)
            if node==3 and gg(8)==get(g+2+gg(7)*2) and gg(7)+1==get(g) and choice:gp(14,1)
            call('garden_step',g,5,0,64)
        assert gg(4)!=1
    gp(3,0);finish(3);assert not gg(3)&4
    finish(2);assert gg(3)==2
    finish(3,1);assert not gg(3)&4
    finish(3);assert gg(3)==6
    finish(1);assert gg(3)==7
    finish(4);assert gg(3)==15 and gg(4)==6
    for area,x,flags,valid in [(1,48,(128<<4)|7,1),(2,64,(128<<4)|2,1),(1,120,(80<<4)|1,0),(2,64,(128<<4)|4,0),(1,48,(128<<4)|8,0)]:assert call('garden_save_valid',area,x,flags)==valid
    # Original renderer uses the shared page painter; only a complete page flips.
    call('garden_init',g);call('ui_painter_init',sym['painter']);put(sym['hz'],60)
    writes=[];early=False;word_reads=0
    def read(cpu,access,address,size,value,user):
        nonlocal word_reads
        if address==0xC00004:cpu.mem_write(address,(8).to_bytes(size,'big'))
        if address==0xC00008:cpu.mem_write(address,(0xE000 if early else 0xE900).to_bytes(size,'big'))
        if 0x200000<=address<0x240000:word_reads+=1
    def write(cpu,access,address,size,value,user):
        if 0xC00000<=address<=0xC00007:writes.append((address,size,value&((1<<(size*8))-1)))
    hr=u.hook_add(UC_HOOK_MEM_READ,read);hw=u.hook_add(UC_HOOK_MEM_WRITE,write)
    call('stream_prepare');pending=get(sym['published']);frame=sym['frames']+(pending-1)*810
    assert pending in (1,2) and get(frame+806)==1
    snapshot=bytes(u.mem_read(frame,810));call('stream_prepare');assert bytes(u.mem_read(frame,810))==snapshot
    call('vblank_user');assert not writes and get(sym['published'])==pending
    early=True
    for i in range(7):
        if i:call('stream_prepare')
        writes.clear();call('vblank_user');call('garden_ack')
        flips=[v for a,n,v in writes if a==0xC00004 and n==2 and v&0xFF00==0x8200]
        assert bool(flips)==(i==6),'Partial UI page became visible'
    assert gg(15)==1
    # Upload bytes are copied while Main owns Word RAM, not retained until IRQ.
    put(0xA12002,1);put(sym['stream_phase'],5);put(sym['stream_uploaded'],0);put(sym['stream_cache_tiles'],20)
    original=bytes(range(256));u.mem_write(0x200040,original);call('stream_prepare');u.mem_write(0x200040,bytes(256))
    reads=word_reads;writes.clear();call('vblank_user')
    data=b''.join(v.to_bytes(n,'big') for a,n,v in writes if a==0xC00000)
    assert data[-256:]==original and word_reads==reads and get(sym['upload_ack'])==256
    assert get(sym['upload_high'])<=784
    call('garden_ack');put(sym['stream_phase'],0)
    put(sym['palettes']+66,0xCEE);call('stream_prepare');pending=get(sym['published'])
    writes.clear();early=False;call('vblank_user');assert not writes and get(sym['published'])==pending
    early=True;call('vblank_user');assert len([1 for a,n,v in writes if a==0xC00004 and n==4 and v&0xC0000030==0xC0000000])==1
    call('garden_ack');writes.clear();call('stream_prepare');call('vblank_user')
    assert not any(a==0xC00004 and n==4 and v&0xC0000030==0xC0000000 for a,n,v in writes)
    call('garden_ack');call('stream_prepare');pending=get(sym['published']);frame=sym['frames']+(pending-1)*810
    put(frame+802,161);writes.clear();call('vblank_user');assert not writes and get(sym['video_fault'])==1
    put(sym['published'],3);writes.clear();call('vblank_user');assert not writes
    u.hook_del(hr);u.hook_del(hw)
    return True

def check_rebuild(project,run):
    """Incremental source-generated art AND header edits must reach real target code."""
    import struct
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC
    font=project/'tools/make_font.py';header=project/'src/garden.h'
    original_font=font.read_text();original_header=header.read_text()
    baseline=(project/'disc/ipx.mmd').read_bytes()
    try:
        font.write_text(original_font.replace("'A':['01110'","'A':['11111'"))
        header.write_text(original_header.replace('G_WALK_STEP 2','G_WALK_STEP 4'))
        run('incremental-mutated',['build','-C',project.name,'REGION=US','VIDEO=NTSC'])
        payload=(project/'disc/ipx.mmd').read_bytes();sy=symbols((project/'build/ipx.mmd.elf.sym').read_text())
        assert payload!=baseline
        def execute(u,sym):
            u.mem_map(0x300000,0x1000);g=sym['garden']
            def call(name,args):
                stack=0xFFDF00;u.reg_write(UC_M68K_REG_A7,stack)
                u.mem_write(stack,struct.pack('>I',0x300000)+b''.join(struct.pack('>I',a) for a in args))
                u.emu_start(sym[name],0x300000,count=1000000)
                assert u.reg_read(UC_M68K_REG_PC)==0x300000
            call('garden_init',[g]);call('garden_enter',[g,0])
            u.mem_write(g+1068,struct.pack('>H',48));u.mem_write(g+1096,struct.pack('>H',1))
            call('garden_step',[g,5,8,0]);assert int.from_bytes(u.mem_read(g+1068,2),'big')==52,'Header change did not reach C object'
            expected=(project/'res/font.bin').read_bytes()
            assert bytes(u.mem_read(sym['res_font'],len(expected)))==expected,'Generated font did not reach assembly object'
            assert expected[(ord('a')-32)*32:(ord('a')-31)*32]!=bytes(32)
            return True
        check(payload,sy,'main',execute)
    finally:
        font.write_text(original_font);header.write_text(original_header)
        run('incremental-restored',['build','-C',project.name,'REGION=US','VIDEO=NTSC'])
    assert (project/'disc/ipx.mmd').read_bytes()==baseline,'Restored inputs did not reproduce original payload'
    return True

"""Execute integration adapters with shared registers and a named-file BIOS model.

No drive, interrupt latency or analog/timing accuracy is claimed.
"""
from startup import check,symbols

def check_practices(project,hz):
    from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_A0,UC_M68K_REG_A1
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    mailbox=bytearray(0x30);mailbox[2:4]=b'\0\1';files={};media=3;write_failure=False;calls=[]
    names=[b'MDBESTLAB_A',b'MDBESTLAB_B']
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
    # Odd-address decoding and equal nominal time without unaligned word loads.
    main.mem_write(game,b'\x00\xfe\xab\xcd');out=game+8
    def aligned_read(cpu,access,address,size,value,user):
        if game<=address<game+4:assert size==1,'Byte decoder emitted a word/long input access'
    alignment_hook=main.hook_add(UC_HOOK_MEM_READ,aligned_read)
    assert call('main','bp_read16',game,4,1,out) and get(main,out)==0xfeab
    main.hook_del(alignment_hook)
    assert call('main','bp_elapsed',3,65533,hz,out) and get(main,out)==step*6
    total=0
    for n in range(1,hz*4+1):call('main','bp_elapsed',n,n-1,hz,out);total+=get(main,out)
    assert total==1200
    def begin(op,values=(1,90,0)):
        game_set(values);assert call('main','checkpoint_request',op,game)==1
        before=bytes(mailbox);assert not call('main','checkpoint_request',op,game) and bytes(mailbox)==before
        assert not call('main','stream_start',0),'Reader stole checkpoint mailbox'
        assert not call('main','audio_request',99,0)
    def retire():
        call('sub','checkpoint_sub_tick');call('main','checkpoint_main_tick',step)
        assert m('checkpoint_pending')==2
        assert not call('main','stream_start',0),'Reader reused unretired reply'
        call('sub','checkpoint_sub_tick')
        assert not call('main','relay_request',0),'Coordinator reused slot before checkpoint retirement'
        call('main','checkpoint_main_tick',step);assert not m('checkpoint_pending')
        return m('checkpoint_result')
    begin(1);assert retire()==4 and not files
    begin(2);game_set((2,200,3));assert retire()==1
    assert list(files)==[names[0]],'Save did not use copied request snapshot'
    begin(1);assert retire()==2 and m('checkpoint_reply')==1 and get(main,ms['checkpoint_reply']+2)==90
    begin(2,(2,201,3));assert retire()==1
    raw=bytearray(files[names[1]]);raw[60]^=1;files[names[1]]=bytes(raw)
    begin(1);assert retire()==3 and m('checkpoint_reply')==1
    write_failure=True;begin(2);assert retire()==6;write_failure=False
    media=1;before=len(calls);begin(2);assert retire()==5 and calls[before:]==[0];media=3
    # BIOS work waits for both the CD reader and an in-flight CD command.
    begin(2);before=len(calls);put(sub,ss['audio_state']+10,1)
    call('sub','checkpoint_sub_tick');assert len(calls)==before and not int.from_bytes(mailbox[0x20:0x22],'big')
    put(sub,ss['audio_state']+10,0);put(sub,ss['access_op'],1,1)
    call('sub','checkpoint_sub_tick');assert len(calls)==before
    put(sub,ss['access_op'],0,1);assert retire()==1
    # Invalid wire operation cannot reach format/delete even if injected.
    mailbox[0x10:0x12]=b'\0\1';mailbox[0x12:0x14]=b'\0\5';before=len(calls)
    call('sub','checkpoint_sub_tick');assert len(calls)==before
    mailbox[0x10:0x12]=b'\0\0';call('sub','checkpoint_sub_tick')
    assert not call('main','checkpoint_request',5,game)
    put(main,ms['stream_phase'],2);before=bytes(mailbox)
    assert not call('main','checkpoint_request',2,game) and bytes(mailbox)==before
    put(main,ms['stream_phase'],0)
    # Save holds the scene generation until the complete reply handshake retires.
    flow=ms['flow'];field=lambda i:get(main,flow+i*2)
    call('main','scene_init',flow);put(main,flow,1);put(main,flow+2,1);put(main,flow+26,1)
    put(main,ms['player_x'],103);put(main,ms['speed'],1)
    put(main,ms['stream_phase'],2)
    assert not call('main','checkpoint_begin',2) and field(9)==0 and m('checkpoint_busy')==1
    put(main,ms['stream_phase'],0)
    assert call('main','checkpoint_begin',2) and field(9)==1 and not m('checkpoint_busy')
    assert call('main','scene_request',flow,0)==1
    call('main','scene_service',120);assert field(3)==2
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    call('main','scene_service',0);assert field(9)==1 and field(3)==2
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    assert not field(9) and not m('checkpoint_pending')
    call('main','scene_service',0);assert field(3)==4
    # A Continue reply cannot override a later menu decision.
    call('main','scene_init',flow);assert call('main','checkpoint_begin',1)
    assert call('main','scene_request',flow,3)==1
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    assert field(1)==3 and not m('restore_pending')
    call('main','scene_init',flow);assert call('main','checkpoint_begin',1)
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    assert field(1)==1 and m('restore_pending')==1
    put(main,flow+6,4);put(main,ms['active'],0);put(main,ms['transition'],0);put(main,ms['published'],0)
    call('main','scene_service',0)
    assert field(0)==1 and m('player_x')==103 and m('speed')==1 and not m('restore_pending')
    # A timed-out BIOS operation retains ownership and the scene lease.
    put(main,flow+6,0);put(main,flow+26,1);assert call('main','checkpoint_begin',2)
    call('main','checkpoint_service',9000)
    assert m('checkpoint_pending')==3 and m('checkpoint_fault')==1 and field(9)==1
    assert not call('main','checkpoint_begin',2)
    call('sub','checkpoint_sub_tick');call('main','checkpoint_service',step)
    assert m('checkpoint_pending')==3 and field(9)==1,'Late completion freed a quarantined slot'
    assert all(op not in (1,5,6) for op in calls)
    return True

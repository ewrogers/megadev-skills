"""Execute both compiled CPUs; model only mailbox/2M ownership, not BIOS timing."""
from startup import check,symbols
def check_system(project,hz,diagnostics=False):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_HOOK_CODE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    sub.mem_map(0x80000,0x40000)
    mailbox=bytearray(0x30);buffer=bytearray(522);mode=1;grant_enabled=True
    trace=[];reads=[];hooks=[]
    def rw_for(cpu,kind,base,ram):
        def read(u,access,address,size,value,user):
            if base<=address<base+0x30:
                off=address-base
                if off<=2<off+size:mailbox[2:4]=mode.to_bytes(2,'big')
                u.mem_write(address,bytes(mailbox[off:off+size]))
            if ram<=address<ram+522:
                assert mode==(1 if kind=='main' else 2),'Read without owned 2M buffer'
                reads.append(kind);u.mem_write(address,bytes(buffer[address-ram:address-ram+size]))
        def write(u,access,address,size,value,user):
            nonlocal mode
            value &= (1<<(size*8))-1
            if base<=address<base+0x30:
                off=address-base
                if off==3:
                    if kind=='main' and value&2 and mode==1 and grant_enabled:mode=2
                    if kind=='sub' and value&1 and mode==2:mode=1
                elif off>=0x10:
                    assert (kind=='main' and off<0x20) or (kind=='sub' and off>=0x20),'Wrong mailbox writer'
                    mailbox[off:off+size]=value.to_bytes(size,'big');trace.append((kind,off,value))
                    if off==0x10 and value:assert mode==2,'Request committed before ownership grant'
                    if off==0x20 and value:
                        assert mode==1,'Reply committed before return'
                        assert int.from_bytes(mailbox[0x2c:0x2e],'big')==value,'Result tag not published before commit'
            if ram<=address<ram+522:
                assert mode==(1 if kind=='main' else 2),'Write without owned 2M buffer'
                buffer[address-ram:address-ram+size]=value.to_bytes(size,'big')
        hooks.extend([cpu.hook_add(UC_HOOK_MEM_READ,read),cpu.hook_add(UC_HOOK_MEM_WRITE,write)])
    rw_for(main,'main',0xA12000,0x200000);rw_for(sub,'sub',0xFF8000,0x80000)
    def word(u,address):return int.from_bytes(u.mem_read(address,2),'big')
    def regword(offset):return int.from_bytes(mailbox[offset:offset+2],'big')
    def call(kind,name,args=()):
        u,sym=contexts[kind];stack=0xFFDF00 if kind=='main' else 0x1FF00;stop=0x300000
        if kind=='main' and name=='sy_main_start':reads.clear()
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((arg&0xffffffff).to_bytes(4,'big') for arg in args))
        try:u.emu_start(sym[name],stop,count=300000)
        except Exception as error:
            pc=u.reg_read(UC_M68K_REG_PC)
            raise AssertionError(f'{kind} {name} PC={pc:x} bytes={bytes(u.mem_read(pc,12)).hex()}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
        return u.reg_read(UC_M68K_REG_D0)
    for u,sym in contexts.values():u.mem_map(0x300000,0x1000)
    now=0
    def frame():
        nonlocal now
        now=(now+1)&65535
        # Vary observation order and omit some worker slices. A repeated tick
        # must not advance another chunk, and an extra zero-time poll is harmless.
        def worker():
            if now%7:
                call('sub','sy_sub_tick',(now,hz))
                before=(bytes(buffer),regword(0x24))
                call('sub','sy_sub_tick',(now,hz))
                assert before==(bytes(buffer),regword(0x24)),'Duplicate worker tick advanced work'
        if now&1:worker()
        call('main','sy_main_tick',(300//hz,))
        if now%3==0:call('main','sy_main_tick',(0,))
        if not now&1:worker()
    # Real Main/Sub adapters exchange input/output, not just the pure state model.
    for profile,slow,cancel,stale in [(0,0,False,False),(1,0,False,False),(2,0,False,False),(0,0,True,False),(0,1,False,False),(0,0,False,True)]:
        assert call('main','sy_main_start',(profile,slow))==1
        token=word(main,ms['client']+2)
        assert not call('main','sy_main_start',(0,0)),'Accepted overlapping request'
        for count in range(300):
            if stale and count==2:mailbox[0x20:0x22]=(token-1).to_bytes(2,'big')
            if cancel and regword(0x24)>=32:call('main','sy_main_cancel')
            frame()
            if not word(main,ms['client']):break
        else:raise AssertionError('Client failed to drain/retire')
        assert mode==1 and not regword(0x10) and not regword(0x20)
    assert word(main,ms['passed'])==2 and word(main,ms['rejected'])==2
    assert word(main,ms['drained'])==2 and not word(main,ms['verify_failed'])
    # A cancelled completed job must be discarded even when the result was valid.
    assert call('main','sy_main_start',(0,0))==1
    call('main','sy_main_tick',(0,))
    for count in range(40):
        now=(now+1)&65535;call('sub','sy_sub_tick',(now,hz))
        if regword(0x20):break
    assert regword(0x20) and word(main,ms['client'])==2
    call('main','sy_main_cancel')
    for count in range(10):
        frame()
        if not word(main,ms['client']):break
    assert not word(main,ms['client'])
    assert word(main,ms['drained'])==3 and word(main,ms['passed'])==2
    if diagnostics:
        # Fault snapshots are private CPU state, not pointers exchanged through Word RAM.
        for kind,table in [('main',ms),('sub',ss)]:
            call(kind,'dbg_clear',(table[kind+'_trace'],))
        assert call('main','sy_main_start',(3,0))==1
        for count in range(80):
            frame()
            if not word(main,ms['client']):break
        else:raise AssertionError('Guard-fault job did not retire')
        assert word(main,ms['rejected'])==3 and not word(main,ms['verify_failed'])
        snapshots={}
        for kind,(u,table) in contexts.items():
            address=table[kind+'_trace']
            # 16 events x 12 bytes, first event x 12, then four words.
            assert word(u,address+210)==1,kind+' fault snapshot not frozen'
            assert word(u,address+194)==(6 if kind=='main' else 3)
            snapshots[kind]=bytes(u.mem_read(address,212))
        assert call('main','sy_main_start',(0,0))==1
        for count in range(80):
            frame()
            if not word(main,ms['client']):break
        assert word(main,ms['passed'])==3
        for kind,(u,table) in contexts.items():
            assert bytes(u.mem_read(table[kind+'_trace'],212))==snapshots[kind], 'Cleanup replaced first-fault evidence'
            call(kind,'dbg_clear',(table[kind+'_trace'],))
        # Freeze on timeout while the worker is still draining, not at a fabricated crash.
        assert call('main','sy_main_start',(0,1))==1
        for count in range(hz+4):frame()
        assert word(main,ms['client'])==3
        assert word(main,ms['main_trace']+194)==4
        frozen=bytes(main.mem_read(ms['main_trace'],212))
        for count in range(hz*2):
            frame()
            if not word(main,ms['client']):break
        assert not word(main,ms['client']) and mode==1
        assert bytes(main.mem_read(ms['main_trace'],212))==frozen
    if diagnostics:
        call('main','dbg_clear',(ms['main_trace'],))
        assert call('main','sy_main_start',(0,0))==1
        call('main','sy_main_tick',(0,))
        for count in range(40):
            now=(now+1)&65535;call('sub','sy_sub_tick',(now,hz))
            if regword(0x20):break
        assert mode==1 and regword(0x20)
        # Corrupt only the modeled returned result, after safe ownership return.
        mailbox[0x2a:0x2c]=(regword(0x2a)^1).to_bytes(2,'big')
        frame()
        assert word(main,ms['verify_failed'])==1
        assert word(main,ms['main_trace']+194)==6
        assert word(main,ms['main_trace']+210)==1
        for count in range(10):
            frame()
            if not word(main,ms['client']):break
        assert not word(main,ms['client'])
    # Word RAM grant failure cannot authorize buffer reuse or an unowned commit.
    grant_enabled=False
    assert call('main','sy_main_start',(0,0))==1
    for count in range(hz+2):frame()
    assert word(main,ms['client'])==5 and not regword(0x10)
    before=bytes(buffer);assert not call('main','sy_main_start',(0,0));assert bytes(buffer)==before
    assert trace and all(off!=0x10 or not value or kind=='main' for kind,off,value in trace)
    # A wrongly committed request must not make Sub read Main-owned Word RAM.
    mailbox[0x10:0x1c]=bytes.fromhex('002a00010007010000000000')
    now=(now+1)&65535;call('sub','sy_sub_tick',(now,hz))
    assert regword(0x20)==42 and regword(0x22)==4 and bytes(buffer)==before
    # The clock and input-edge queue run on every IRQ, even with no UI frame.
    import unicorn.m68k_const as regs
    for address in [0x360,0x298]:main.mem_write(address,b'\x4e\x75')
    calls=[]
    def bios_call(u,address,size,user):
        if address in (0x360,0x298):calls.append(address)
    hook=main.hook_add(UC_HOOK_CODE,bios_call)
    saved={getattr(regs,'UC_M68K_REG_'+name):0x12340000+i for i,name in enumerate([*[f'D{i}' for i in range(8)],*[f'A{i}' for i in range(7)]])}
    for hit in [1,64]:
        for reg,value in saved.items():main.reg_write(reg,value)
        main.reg_write(UC_M68K_REG_SR,0x2700);main.reg_write(UC_M68K_REG_A7,0xFFDF00)
        main.mem_write(0xFFDF00,bytes.fromhex('200000300000'));main.mem_write(0xFFFE21,bytes([hit]))
        end=ms['system_irq_return'];main.emu_start(ms['system_irq'],end,count=10000)
        assert main.reg_read(UC_M68K_REG_PC)==end and main.reg_read(UC_M68K_REG_A7)==0xFFDF00
        assert bytes(main.mem_read(end,2))==bytes.fromhex('4e73')
        assert bytes(main.mem_read(0xFFDF00,6))==bytes.fromhex('200000300000')
        assert all(main.reg_read(reg)==value for reg,value in saved.items())
    assert calls==[0x360,0x298,0x360,0x298]
    assert word(main,ms['main_ticks'])==2 and word(main,ms['input_hits'])==65
    main.hook_del(hook)
    return True

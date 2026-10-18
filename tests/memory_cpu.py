"""Paired original-68000 memory adapters with modeled physical Word RAM mapping."""
from startup import check,symbols
def check_memory(project,hz):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    sub.mem_map(0x80000,0x60000)
    for u,sym in contexts.values():u.mem_map(0x300000,0x1000)
    mailbox=bytearray(0x30);physical=bytearray(262144)
    one=False;select=0;switch=0;request=0;grants=True
    transfers=[];reads={'main':0,'sub':0};writes={'main':0,'sub':0}
    def mode():return (4|select|(request<<1)) if one else (2 if switch else 1)
    def physical_offsets(kind,address,size):
        if kind=='main':
            assert 0x200000<=address and address+size<=0x220000,'Main used the cell-mapped alias as a second bank'
            offset=address-0x200000
            if not one:assert not switch,'Main accessed Sub-owned 2M RAM'
            bank=select
        else:
            base=0xC0000 if one else 0x80000
            assert base<=address and address+size<=base+(131072 if one else 262144),'Sub used the wrong mode view'
            offset=address-base;bank=select^1
            if not one:assert switch,'Sub accessed Main-owned 2M RAM'
        # 1M banks are interleaved physical words, not two contiguous 2M halves.
        return [((((offset+i)>>1)*2+bank)*2+((offset+i)&1)) if one else offset+i for i in range(size)]
    def install(u,kind,base):
        def read(cpu,access,address,size,value,user):
            if base<=address<base+0x30:
                off=address-base;mailbox[2:4]=mode().to_bytes(2,'big');u.mem_write(address,bytes(mailbox[off:off+size]))
            if (kind=='main' and 0x200000<=address<0x240000) or (kind=='sub' and 0x80000<=address<0xE0000):
                offsets=physical_offsets(kind,address,size);u.mem_write(address,bytes(physical[i] for i in offsets));reads[kind]+=1
        def write(cpu,access,address,size,value,user):
            nonlocal one,select,switch,request
            value&=(1<<(size*8))-1
            if base<=address<base+0x30:
                off=address-base
                if off==3:
                    if grants:
                        if kind=='main':
                            if value&2:switch=1
                            else:request=1
                        else:
                            if select!=(value&1):request=0
                            select=value&1;one=bool(value&4)
                            if select:switch=0
                        transfers.append((kind,mode()))
                elif off>=0x10:
                    assert (kind=='main' and off<0x20) or (kind=='sub' and off>=0x20),'Wrong mailbox writer'
                    mailbox[off:off+size]=value.to_bytes(size,'big')
                    if off==0x20 and value:assert int.from_bytes(mailbox[0x2c:0x2e],'big')==value
            if (kind=='main' and 0x200000<=address<0x240000) or (kind=='sub' and 0x80000<=address<0xE0000):
                for index,byte in zip(physical_offsets(kind,address,size),value.to_bytes(size,'big')):physical[index]=byte
                writes[kind]+=1
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    install(main,'main',0xA12000);install(sub,'sub',0xFF8000)
    def word(u,address):return int.from_bytes(u.mem_read(address,2),'big')
    def call(kind,name,args=()):
        u,sym=contexts[kind];stack=0xFFDF00 if kind=='main' else 0x1FF00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
        try:u.emu_start(sym[name],stop,count=400000)
        except Exception as error:
            pc=u.reg_read(UC_M68K_REG_PC);raise AssertionError(f'{kind} {name}: PC {pc:x}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
        return u.reg_read(UC_M68K_REG_D0)
    now=0
    def worker():
        if now%7:
            call('sub','memory_sub_tick',(now,hz));before=(bytes(physical),bytes(mailbox))
            call('sub','memory_sub_tick',(now,hz));assert before==(bytes(physical),bytes(mailbox))
    def frame():
        nonlocal now
        now=(now+1)&65535
        if now&1:worker()
        call('main','memory_main_tick',(300//hz,))
        if not now&1:worker()
        if word(main,ms['front_valid']):assert call('main','memory_front_check')==1
    # Same pointer window must resolve to the newly assigned physical bank.
    operations=[1,2,4,3,4,3,4,5,1,2,5]
    for index,op in enumerate(operations):
        if op==4:
            lease=0xFFE000;main.mem_write(lease,bytes(8))
            assert call('main','mv_pin',(ms['word_view'],lease))==1
            before=(mode(),bytes(mailbox),bytes(physical))
            assert not call('main','memory_start',(op,))
            assert before==(mode(),bytes(mailbox),bytes(physical)),'Pinned bank was exchanged'
            assert call('main','mv_release',(ms['word_view'],lease))==1
        assert call('main','memory_start',(op,))==1
        generation=int.from_bytes(main.mem_read(ms['word_view'],4),'big')
        assert not call('main','memory_start',(op,)),'Accepted overlapping memory operation'
        start_reads=reads['main'];start_sub=writes['sub'];concurrent_reads=False
        for count in range(300):
            frame()
            if op==3 and writes['sub']>start_sub and reads['main']>start_reads:concurrent_reads=True
            if not word(main,ms['memory_phase']):break
        else:raise AssertionError(f'Operation {op} failed, phase {word(main,ms["memory_phase"])} mode {mode()}')
        assert not word(main,ms['memory_errors']) and call('main','memory_front_check')==1
        assert word(main,ms['memory_pass'])==index+1
        assert int.from_bytes(main.mem_read(ms['word_view'],4),'big')==generation
        if op==3:assert concurrent_reads,'Did not check Main front while Sub filled back'
        if op in (1,5):assert mode()==1
    # Actual arena demonstration refuses unload while a queued user holds it.
    call('main','ma_init',(ms['arena'],4096,1))
    for step in range(5):call('main','life_step')
    assert word(main,ms['life_failures'])==1 and word(main,ms['life_blocks'])==1
    assert word(main,ms['stale_rejected'])==1 and not word(main,ms['video_fault'])
    # Releasing early through the manual control makes unload legal, not a fault.
    main.mem_write(ms['life_step_index'],bytes(2));call('main','life_step')
    assert call('main','ma_release',(ms['arena'],ms['scene_lease']))==1
    call('main','life_step');call('main','life_step')
    assert word(main,ms['life_step_index'])==5 and not word(main,ms['video_fault'])
    # A grant that never confirms leaves the view isolated; no speculative RAM reads.
    grants=False;before=bytes(physical)
    assert call('main','memory_start',(1,))==1
    for count in range(hz*5+2):frame()
    assert word(main,ms['memory_phase'])==4 and bytes(physical)==before
    assert not call('main','memory_start',(1,))
    assert transfers and any(value&4 for kind,value in transfers)
    return True

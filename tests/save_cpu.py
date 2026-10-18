"""Execute original-68000 save service and ABI with a named-file BIOS model."""
import zlib
from startup import check,symbols
def check_save(project,hz):
    from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_A0,UC_M68K_REG_A1
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    mailbox=bytearray(0x30);files={};cart_files={};media=3;write_failure=False;read_failure=False;bad_verify=False;writes=0;reads=0
    meta={};calls=[];allow_format=False;format_failure=False;format_count=0;post_format_media=3
    names=[b'MDSAVELAB_A',b'MDSAVELAB_B']
    for kind,(u,sym) in contexts.items():
        u.mem_map(0x300000,0x1000)
        base=0xA12000 if kind=='main' else 0xFF8000
        def read(cpu,access,address,size,value,user,base=base):
            if base<=address<base+0x30:cpu.mem_write(address,bytes(mailbox[address-base:address-base+size]))
        def write(cpu,access,address,size,value,user,base=base,kind=kind):
            if base<=address<base+0x30:
                off=address-base
                assert (kind=='main' and 0x10<=off<0x20) or (kind=='sub' and off>=0x20 and off!=0x26),'Wrong register owner'
                mailbox[off:off+size]=(value&((1<<(size*8))-1)).to_bytes(size,'big')
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    sub,ss=contexts['sub'];main,ms=contexts['main']
    def word(u,p,n=2):return int.from_bytes(u.mem_read(p,n),'big')
    def put(u,p,x,n=2):u.mem_write(p,x.to_bytes(n,'big'))
    def bios(u,address,size,user):
        nonlocal writes,reads,media,format_count
        table=ss if u is sub else ms;store=files if u is sub else cart_files
        op=u.reg_read(UC_M68K_REG_D0)&65535;a0=u.reg_read(UC_M68K_REG_A0);a1=u.reg_read(UC_M68K_REG_A1);carry=False
        calls.append(op);assert op in [0,2,3,4,6],'Unexpected BIOS call, including delete'
        if op==6:assert allow_format,'Formatting outside explicit test confirmation'
        if op==0:
            assert table['_BSS_ORIGIN']<=a0 and a0+0x640<=table['_BSS_ORIGIN']+table['_BSS_LENGTH'],'BRMINIT work buffer not resident'
            assert table['_BSS_ORIGIN']<=a1 and a1+12<=table['_BSS_ORIGIN']+table['_BSS_LENGTH']
            u.reg_write(UC_M68K_REG_D0,2);u.reg_write(UC_M68K_REG_D1,0 if media==3 else media);carry=media!=3
        elif op==6:
            format_count+=1;carry=format_failure
            if not carry:store.clear();media=post_format_media
        else:
            name=bytes(u.mem_read(a0,11));assert name in names,'Touched unrelated save or failed to reset A0 after search'
            if op==2:
                carry=name not in store
                count,mode=meta.get(name,(1,0));u.reg_write(UC_M68K_REG_D0,count);u.reg_write(UC_M68K_REG_D1,mode)
                u.reg_write(UC_M68K_REG_A0,0xFE0001)
            elif op==3:
                reads+=1;carry=read_failure or name not in store
                if not carry:
                    u.mem_write(a1,store[name]);u.reg_write(UC_M68K_REG_D0,1);u.reg_write(UC_M68K_REG_D1,0)
            else:
                assert word(u,a0+11,1)==0 and word(u,a0+12)==1,'Write metadata size/mode wrong'
                assert u.reg_read(UC_M68K_REG_D1)==0,'BRMWRITE requires D1.l zero'
                data=bytearray(u.mem_read(a1,64));writes+=1
                if write_failure or bad_verify:data[60]^=1
                store[name]=bytes(data);carry=write_failure
        sr=u.reg_read(UC_M68K_REG_SR);u.reg_write(UC_M68K_REG_SR,(sr|1) if carry else sr&~1)
        stack=u.reg_read(UC_M68K_REG_A7);u.reg_write(UC_M68K_REG_PC,word(u,stack,4));u.reg_write(UC_M68K_REG_A7,stack+4)
    sub.hook_add(UC_HOOK_CODE,bios,begin=0x5F16,end=0x5F16)
    main.mem_map(0x400000,0x1000)
    main.mem_write(0xFFFDAE,bytes.fromhex('4ef9000088f0'))
    main.hook_add(UC_HOOK_CODE,bios,begin=0xFFFDAE,end=0xFFFDAE)
    def call(kind,name,*args):
        u,sym=contexts[kind];stack=0xFFDF00 if kind=='main' else 0x1FF00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((a&0xffffffff).to_bytes(4,'big') for a in args))
        u.emu_start(sym[name],stop,count=2000000)
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' damaged stack'
        return u.reg_read(UC_M68K_REG_D0)
    game=0xFFDC00
    def set_game(level,coins,flags):main.mem_write(game,b''.join(x.to_bytes(2,'big') for x in [level,coins,flags]))
    def send(op,values=(2,17,1)):
        set_game(*values);assert call('main','save_request',op,game)==1
        before=bytes(mailbox);assert not call('main','save_request',op,game);assert bytes(mailbox)==before
        call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
        assert word(main,ms['save_pending'])==2
        call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
        assert not word(main,ms['save_pending'])
        return word(main,ms['save_result'])
    assert send(1)==4 and writes==0
    assert send(2)==1 and writes==1
    data=files[names[0]];assert int.from_bytes(data[60:],'big')==zlib.crc32(data[:60])
    assert send(2,(4,31,3))==1 and writes==2
    assert send(3)==2 and word(main,ms['save_reply'])==4
    assert send(4)==3 and word(main,ms['save_reply'])==2,'Damaged newest did not recover older'
    assert send(2,(5,41,2))==1
    write_failure=True;assert send(2,(6,51,3))==6;write_failure=False
    assert send(3)==3 and word(main,ms['save_reply'])==5
    bad_verify=True;assert send(2)==10;bad_verify=False
    read_failure=True;before=writes;assert send(2)==6 and writes==before;read_failure=False
    # Oversized/protected file is rejected before BIOS can overrun a 64-byte buffer.
    for bad in [(2,0),(1,255)]:
        meta[names[0]]=bad;before_reads=reads;assert send(2)==9 and writes==before
        assert reads-before_reads<=1,'Unsafe file passed metadata gate'
    meta.clear()
    for unavailable in [0,1,2]:
        media=unavailable;before_calls=len(calls);assert send(2)==5 and writes==before
        assert calls[before_calls:]==[0],'Unavailable media reached file operations'
    media=3;set_game(1,0,0);assert not call('main','save_request',77,game)
    # Standard-cart backend runs on Main, uses an independent file namespace
    # and refuses absent/custom-handler IDs before dispatching any BIOS code.
    put(main,ms['save_device'],1)
    for ident in [0,7,0x80,0xff]:
        main.mem_write(0x400001,bytes([ident]));before_calls=len(calls)
        assert call('main','save_request',2,game)==1
        assert word(main,ms['save_result'])==5 and len(calls)==before_calls
    main.mem_write(0x400001,b'\4');main.mem_write(0xFFFDB0,(0x400020).to_bytes(4,'big'))
    before_calls=len(calls);assert call('main','save_request',2,game)==1
    assert word(main,ms['save_result'])==5 and len(calls)==before_calls,'Cartridge-provided handler was executed'
    main.mem_write(0xFFFDB0,(0x88f0).to_bytes(4,'big'))
    for ident in [1,4,6]:
        main.mem_write(0x400001,bytes([ident]));set_game(7,77,2)
        assert call('main','save_request',2,game)==1 and word(main,ms['save_result'])==1
    set_game(1,0,0);assert call('main','save_request',3,game)==1
    assert word(main,ms['save_result'])==2 and word(main,ms['save_reply'])==7
    assert call('main','save_request',4,game)==1 and word(main,ms['save_result'])==3
    before=writes;media=1
    assert call('main','save_request',2,game)==1 and word(main,ms['save_result'])==5 and writes==before
    media=3;put(main,ms['save_device'],0)
    # Device switching is explicit and an absent cart never silently saves to
    # internal BRAM. Returning to internal probes through the Sub service.
    main.mem_write(0x400001,b'\0');put(main,ms['field'],3);put(main,ms['help'],0)
    before=writes;call('main','save_input',1<<3)
    assert word(main,ms['save_device'])==1 and word(main,ms['save_result'])==5 and writes==before
    assert not int.from_bytes(mailbox[0x10:0x12],'big')
    call('main','save_input',1<<3);assert word(main,ms['save_device'])==0
    call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
    call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
    # Actual menu dispatch: Help's B returns without accidentally starting a
    # load; Help's A explicitly requests recovery without a held-button race.
    put(main,ms['help'],1);call('main','save_input',1<<4)
    assert not word(main,ms['help']) and not word(main,ms['save_pending'])
    put(main,ms['help'],1);call('main','save_input',1<<6)
    assert int.from_bytes(mailbox[0x18:0x1a],'big')==4 and word(main,ms['save_pending'])==1
    before=bytes(mailbox);call('main','save_input',1<<6);assert bytes(mailbox)==before
    call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
    call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
    assert not word(main,ms['save_pending'])
    # Confirmation defaults to Cancel on every open; selecting a device or
    # opening Help never formats. Back/Start and simultaneous navigation+A
    # cannot bypass confirmation. While pending, controls cannot arm a format.
    put(main,ms['help'],1)
    before_calls=len(calls)
    call('main','save_input',1<<5) # C opens format confirmation
    assert word(main,ms['format_dialog'])==1 and not word(main,ms['format_yes'])
    call('main','save_input',1<<6) # A accepts default Cancel
    assert not word(main,ms['format_dialog']) and len(calls)==before_calls
    for cancel in [1<<4,1<<7]:
        call('main','save_input',1<<5);call('main','save_input',1<<3)
        call('main','save_input',cancel)
        assert not word(main,ms['format_dialog']) and len(calls)==before_calls
    call('main','save_input',1<<5)
    call('main','save_input',(1<<3)|(1<<6))
    assert word(main,ms['format_dialog'])==1 and len(calls)==before_calls
    call('main','save_input',1<<4)
    # Confirmed internal format clears all files, not only Lab records, and
    # re-probes. The cart's independently stored files remain unchanged.
    unrelated=b'OTHERGAME_X';files[unrelated]=b'foreign';cart_before=dict(cart_files)
    call('main','save_input',1<<5);call('main','save_input',1<<3)
    put(main,ms['format_device'],1);call('main','save_input',1<<6)
    assert not word(main,ms['save_pending']) and len(calls)==before_calls,'Device mismatch confirmed format'
    allow_format=True;call('main','save_input',1<<5);call('main','save_input',1<<3)
    call('main','save_input',1<<6)
    assert word(main,ms['save_pending'])==1
    before=bytes(mailbox);call('main','save_input',1<<5);assert bytes(mailbox)==before
    call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
    call('sub','save_sub_tick');call('main','save_main_tick',300//hz)
    assert word(main,ms['save_result'])==13 and not files and cart_files==cart_before
    assert word(main,ms['save_sequence'])==0 and format_count==1
    assert calls[-3:]==[0,6,0],'Format must initialize then re-probe'
    assert send(2)==1 and send(3)==2,'Internal saves failed after format'
    # Absent media refuses format; unformatted/foreign media require explicit
    # op 5. BIOS failure and failed post-format probe never report success.
    for status in [0,1,2]:
        media=status;before_format=format_count
        assert send(2)==5 and format_count==before_format
        assert send(5)==(5 if status==0 else 13)
        assert format_count==before_format+(status!=0)
    media=3;format_failure=True;assert send(5)==6;format_failure=False
    post_format_media=1;assert send(5)==5;post_format_media=3;media=3
    # Same explicit flow on Main/cart, with independent store and no fallback.
    put(main,ms['save_device'],1);main.mem_write(0x400001,b'\4')
    cart_files[unrelated]=b'foreign';internal_before=dict(files)
    call('main','save_input',1<<5);assert not word(main,ms['format_yes'])
    call('main','save_input',1<<3);call('main','save_input',1<<6)
    assert word(main,ms['save_result'])==13 and not cart_files and files==internal_before
    assert word(main,ms['save_sequence'])==0 and calls[-3:]==[0,6,0]
    assert call('main','save_request',2,game)==1 and word(main,ms['save_result'])==1
    assert call('main','save_request',3,game)==1 and word(main,ms['save_result'])==2
    for status in [1,2]:
        media=status;assert call('main','save_request',5,game)==1
        assert word(main,ms['save_result'])==13
    format_failure=True;assert call('main','save_request',5,game)==1
    assert word(main,ms['save_result'])==6;format_failure=False
    post_format_media=1;assert call('main','save_request',5,game)==1
    assert word(main,ms['save_result'])==5;post_format_media=3;media=3
    for ident in [0,0x80]:
        main.mem_write(0x400001,bytes([ident]));before_format=format_count
        assert call('main','save_request',5,game)==1
        assert word(main,ms['save_result'])==5 and format_count==before_format
    allow_format=False;put(main,ms['save_device'],0)
    assert call('main','save_request',3,game)
    for _ in range(hz*30+1):call('main','save_main_tick',300//hz)
    assert word(main,ms['save_pending'])==3 and word(main,ms['save_fault'])==1
    assert not call('main','save_request',2,game),'Lost reply freed request slot'
    return True

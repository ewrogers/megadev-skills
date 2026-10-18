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
    mailbox=bytearray(0x30);files={};media=3;write_failure=False;read_failure=False;bad_verify=False;writes=0;reads=0
    meta={};calls=[]
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
        nonlocal writes,reads
        op=u.reg_read(UC_M68K_REG_D0)&65535;a0=u.reg_read(UC_M68K_REG_A0);a1=u.reg_read(UC_M68K_REG_A1);carry=False
        calls.append(op);assert op in [0,2,3,4],'Unexpected BIOS call, including delete/format'
        if op==0:
            assert ss['_BSS_ORIGIN']<=a0 and a0+0x640<=ss['_BSS_ORIGIN']+ss['_BSS_LENGTH'],'BRMINIT work buffer not resident'
            assert ss['_BSS_ORIGIN']<=a1 and a1+12<=ss['_BSS_ORIGIN']+ss['_BSS_LENGTH']
            u.reg_write(UC_M68K_REG_D0,2);u.reg_write(UC_M68K_REG_D1,0 if media==3 else media);carry=media!=3
        else:
            name=bytes(u.mem_read(a0,11));assert name in names,'Touched unrelated save or failed to reset A0 after search'
            if op==2:
                carry=name not in files
                count,mode=meta.get(name,(1,0));u.reg_write(UC_M68K_REG_D0,count);u.reg_write(UC_M68K_REG_D1,mode)
                u.reg_write(UC_M68K_REG_A0,0xFE0001)
            elif op==3:
                reads+=1;carry=read_failure or name not in files
                if not carry:
                    u.mem_write(a1,files[name]);u.reg_write(UC_M68K_REG_D0,1);u.reg_write(UC_M68K_REG_D1,0)
            else:
                assert word(u,a0+11,1)==0 and word(u,a0+12)==1,'Write metadata size/mode wrong'
                assert u.reg_read(UC_M68K_REG_D1)==0,'BRMWRITE requires D1.l zero'
                data=bytearray(u.mem_read(a1,64));writes+=1
                if write_failure or bad_verify:data[60]^=1
                files[name]=bytes(data);carry=write_failure
        sr=u.reg_read(UC_M68K_REG_SR);u.reg_write(UC_M68K_REG_SR,(sr|1) if carry else sr&~1)
        stack=u.reg_read(UC_M68K_REG_A7);u.reg_write(UC_M68K_REG_PC,word(u,stack,4));u.reg_write(UC_M68K_REG_A7,stack+4)
    sub.hook_add(UC_HOOK_CODE,bios,begin=0x5F16,end=0x5F16)
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
    assert call('main','save_request',3,game)
    for _ in range(hz*30+1):call('main','save_main_tick',300//hz)
    assert word(main,ms['save_pending'])==3 and word(main,ms['save_fault'])==1
    assert not call('main','save_request',2,game),'Lost reply freed request slot'
    return True

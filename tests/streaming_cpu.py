"""Execute both compiled adapters; modeled drive writes are not BIOS/seek timing evidence."""
from startup import check,symbols

def check_streaming(project,hz):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    boot=symbols((project/'build/sp.bin.elf.sym').read_text());sub.mem_write(0x6000,(project/'build/sp.bin').read_bytes())
    sub.mem_map(0x80000,0x40000)
    for u,sym in contexts.values():u.mem_map(0x300000,0x1000)
    def put(u,address,value,size=2):u.mem_write(address,value.to_bytes(size,'big'))
    def word(u,address,size=2):return int.from_bytes(u.mem_read(address,size),'big')
    for name in ['access_op','access_op_result']:put(sub,ss[name],0)
    for name in ['filename','filebuff','filesize']:put(sub,ss[name],0,4)
    files=[(project/'res'/f'{name}.mda').read_bytes() for name in ['garden','cavern']]
    # Real pinned find_file + our C ABI shim operate on genuine 22-byte cache records.
    entries=b''.join(name.ljust(14,b'\0')+(20+i*16).to_bytes(4,'big')+len(data).to_bytes(4,'big')
        for i,(name,data) in enumerate(zip([b'GARDEN.MDA;1',b'CAVERN.MDA;1'],files)))
    sub.mem_write(boot['dir_cache'],entries);put(sub,boot['dir_entry_count'],2)
    mailbox=bytearray(0x30);owner='main';grants=True;physical=bytearray(262144)
    reads=0;writes=0
    def install(u,kind,base):
        def read(cpu,access,address,size,value,user):
            nonlocal reads
            if base<=address<base+0x30:
                off=address-base;mailbox[2:4]=(1 if owner=='main' else 2).to_bytes(2,'big');u.mem_write(address,bytes(mailbox[off:off+size]))
            if (kind=='main' and 0x200000<=address<0x240000) or (kind=='sub' and 0x80000<=address<0xC0000):
                assert owner==kind,'Read from other CPU-owned Word RAM';offset=address-(0x200000 if kind=='main' else 0x80000)
                u.mem_write(address,bytes(physical[offset:offset+size]));reads+=1
        def write(cpu,access,address,size,value,user):
            nonlocal owner,writes
            value&=(1<<(size*8))-1
            if base<=address<base+0x30:
                off=address-base
                if off==3 and grants:
                    if kind=='main' and value&2:owner='sub'
                    elif kind=='sub' and value&1:owner='main'
                elif off>=0x10:
                    assert (kind=='main' and off<0x20) or (kind=='sub' and off>=0x20),'Wrong mailbox writer'
                    mailbox[off:off+size]=value.to_bytes(size,'big')
                    if off==0x20 and value:
                        assert int.from_bytes(mailbox[0x2c:0x2e],'big')==value,'Reply token published before metadata'
            if kind=='sub' and 0x80000<=address<0xC0000:
                assert owner=='sub';offset=address-0x80000;physical[offset:offset+size]=value.to_bytes(size,'big');writes+=1
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    install(main,'main',0xA12000);install(sub,'sub',0xFF8000)
    def call(kind,name,args=()):
        u,sym=contexts[kind];stack=0xFFDF00 if kind=='main' else 0x1FF00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
        try:u.emu_start(sym[name],stop,count=1000000)
        except Exception as error:raise AssertionError(f'{kind} {name}: PC {u.reg_read(UC_M68K_REG_PC):x}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
        return u.reg_read(UC_M68K_REG_D0)
    now=0;pending=None;drive_failure=False;drive_delay=5;corrupt=False;uploads=[]
    def drive():
        nonlocal pending,writes
        if word(sub,ss['access_op'])==3:
            if pending is None:
                ptr=word(sub,ss['filename'],4);name=bytes(sub.mem_read(ptr,14)).split(b'\0')[0]
                index=0 if name==b'GARDEN.MDA;1' else 1;data=files[index]
                pending=[drive_delay,0,data+(b'\0'*((-len(data))%2048))]
            assert owner=='sub','Ownership returned while CDC still active'
            if pending[0]:pending[0]-=1;return
            start=pending[1];end=min(start+512,len(pending[2]));physical[start:end]=pending[2][start:end];pending[1]=end;writes+=end-start
            if end==len(pending[2]):
                if corrupt:physical[100]^=1
                put(sub,ss['filesize'],len(files[word(sub,ss['asset'])]),4)
                put(sub,ss['access_op_result'],0xff9c if drive_failure else 0x64);put(sub,ss['access_op'],0);pending=None
    def frame(upload=True):
        nonlocal now
        now=(now+1)&65535;drive()
        if now%7:call('sub','stream_sub_tick',(now,hz))
        call('main','stream_main_tick',(300//hz,))
        if upload and word(main,ms['stream_phase'])==5:
            assert owner=='main';offset=word(main,ms['stream_uploaded']);tiles=word(main,ms['stream_cache_tiles'])
            count=min(64 if now&1 else 256,tiles*32-offset)
            assert not word(main,ms['stream_ready']),'Partial upload was published'
            uploads.append(bytes(physical[64+offset:64+offset+count]));call('main','stream_upload_done',(count,))
    def finish():
        for i in range(1800):
            frame()
            if not word(main,ms['stream_phase']):return
        raise AssertionError('Request did not retire')
    for index in [0,1,0]:
        uploads.clear();assert call('main','stream_start',(index,))==1
        assert not call('main','stream_start',(index^1,)),'Overlapping request accepted'
        finish();assert word(main,ms['stream_ready'])==1,(index,word(main,ms['stream_result']),word(main,ms['stream_errors']))
        assert b''.join(uploads)==files[index][64:64+word(main,ms['stream_cache_tiles'])*32]
        assert bytes(main.mem_read(ms['stream_cache_map'],1600))==files[index][-1600:]
    # Entry prefetch runs before reaching the exit and is consumed exactly once.
    put(main,ms['active'],0);put(main,ms['player_x'],40);put(main,ms['preload_on_entry'],1)
    call('main','stream_preload_entry');assert word(main,ms['stream_phase'])==1
    assert word(main,ms['stream_asset'])==1 and not word(main,ms['preload_on_entry'])
    finish();call('main','stream_preload_entry');assert not word(main,ms['stream_phase'])
    # An exit wait is explicit only at the boundary, unpaused, with a pending destination.
    put(main,ms['want_warp'],1);assert not call('main','stream_exit_waiting')
    put(main,ms['player_x'],294);assert call('main','stream_exit_waiting')==1
    put(main,ms['paused'],1);assert not call('main','stream_exit_waiting')
    put(main,ms['paused'],0);put(main,ms['cached']+2,1);assert not call('main','stream_exit_waiting')
    put(main,ms['cached']+2,0);put(main,ms['want_warp'],0)
    # Cancellation while the drive writes: no second request and no partial publication.
    drive_delay=20;assert call('main','stream_start',(1,))==1
    for i in range(12):frame()
    assert word(sub,ss['access_op'])==3
    call('main','stream_cancel');assert not call('main','stream_start',(0,))
    finish();assert not word(main,ms['stream_ready']) and word(main,ms['stream_result'])==2 and owner=='main'
    # Cancellation after the reader retired also drops staged validation/upload data.
    for phase in [4,5]:
        assert call('main','stream_start',(0,))==1
        for i in range(1000):
            frame(upload=False)
            if word(main,ms['stream_phase'])==phase:break
        else:raise AssertionError('Did not reach staged phase')
        call('main','stream_cancel')
        assert not word(main,ms['stream_phase']) and not word(main,ms['stream_ready']) and owner=='main'
    # Missing, rejected directory size, reader failure and corrupt payload remain recoverable.
    assert call('main','stream_start',(2,))==1;finish();assert word(main,ms['stream_result'])==3
    put(sub,boot['dir_cache']+18,32769,4)
    assert call('main','stream_start',(0,))==1;finish();assert word(main,ms['stream_result'])==4
    put(sub,boot['dir_cache']+18,len(files[0]),4)
    drive_failure=True;assert call('main','stream_start',(0,))==1;finish();assert word(main,ms['stream_result'])==5
    drive_failure=False;corrupt=True;assert call('main','stream_start',(0,))==1;finish()
    assert word(main,ms['stream_result'])==6 and not word(main,ms['stream_ready'])
    corrupt=False
    # A live reader outlasting Main's deadline drains; it never authorizes reuse.
    drive_delay=hz*31;assert call('main','stream_start',(1,))==1
    for i in range(hz*30+4):frame()
    assert word(main,ms['stream_phase'])==6 and owner=='sub' and not call('main','stream_start',(0,))
    finish();assert word(main,ms['stream_result'])==7 and not word(main,ms['stream_ready'])
    assert call('main','stream_start',(0,))==1;drive_delay=0;finish();assert word(main,ms['stream_ready'])
    # Publisher snapshots Word RAM bytes before VBlank; no queued pointer survives a grant.
    main.mem_map(0xC00000,0x10000)
    put(main,ms['stream_phase'],5);put(main,ms['stream_uploaded'],0);put(main,ms['stream_cache_tiles'],438)
    put(main,ms['loaded_slot'],0);put(main,ms['active'],0)
    expected=bytes(physical[64:64+256]);call('main','stream_prepare')
    physical[64:64+256]=bytes(256);written=[]
    def vdp_write(u,access,address,size,value,user):written.append((address,size,value&((1<<(size*8))-1)))
    hook=main.hook_add(UC_HOOK_MEM_WRITE,vdp_write,begin=0xC00000,end=0xC00007)
    before_reads=reads;call('main','vblank_user')
    assert reads==before_reads,'VBlank retained a Word RAM pointer'
    payload=b''.join(value.to_bytes(size,'big') for address,size,value in written[-128:])
    assert payload==expected,'Publisher did not keep immutable copied bytes'
    assert word(main,ms['upload_ack'])==256 and not word(main,ms['published'])
    before=len(written);call('main','vblank_user');assert len(written)==before,'Frame submitted twice'
    # CRAM is never written during active display or late blank; unchanged lines stay resident.
    put(main,ms['stream_phase'],0);put(main,ms['palettes']+33*2,0x0eec)
    blank=False;vertical=224
    def vdp_read(u,access,address,size,value,user):
        if address==0xC00004:u.mem_write(address,(8 if blank else 0).to_bytes(2,'big'))
        if address==0xC00008:u.mem_write(address,(vertical<<8).to_bytes(2,'big'))
    rh=main.hook_add(UC_HOOK_MEM_READ,vdp_read,begin=0xC00004,end=0xC00009)
    def cram_commands():return [v for a,n,v in written if a==0xC00004 and n==4 and (v&0xC0000030)==0xC0000000]
    written.clear();call('main','stream_prepare');call('main','vblank_user');assert not cram_commands()
    blank=True;vertical=235
    written.clear();call('main','stream_prepare');call('main','vblank_user');assert not cram_commands()
    vertical=224
    written.clear();call('main','stream_prepare');call('main','vblank_user');assert len(cram_commands())==1,[(hex(a),n,hex(v)) for a,n,v in written if a==0xC00004]
    written.clear();call('main','stream_prepare');call('main','vblank_user');assert not cram_commands()
    main.hook_del(rh);main.hook_del(hook)
    # Unconfirmed grant: no speculative read or write, and no buffer reuse.
    grants=False;before=(bytes(physical),reads,writes);assert call('main','stream_start',(1,))==1
    for i in range(hz*30+2):frame()
    assert word(main,ms['stream_phase'])==6 and (bytes(physical),reads,writes)==before
    assert not call('main','stream_start',(0,))
    return True

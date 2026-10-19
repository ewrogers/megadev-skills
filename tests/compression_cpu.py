"""Execute the real Kosinski assembly and both compiled loading adapters.

CDC completion and stopwatch reads are modeled, not drive/cycle measurements.
"""
from startup import check,symbols
from test_compression import vectors,tool

def check_compression(project,hz):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn import m68k_const as mc
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    main.mem_map(0xc00000,0x1000)
    ports=[]
    def vdp_write(cpu,access,address,size,value,user):
        if address==0xc00000 and size==2:ports.append((value&65535).to_bytes(2,'big'))
    main.hook_add(UC_HOOK_MEM_WRITE,vdp_write,begin=0xc00000,end=0xc00001)
    boot=symbols((project/'build/sp.bin.elf.sym').read_text());sub.mem_write(0x6000,(project/'build/sp.bin').read_bytes())
    sub.mem_map(0x80000,0x40000)
    for u,sym in contexts.values():u.mem_map(0x300000,0x1000)
    def get(u,address,size=2):return int.from_bytes(u.mem_read(address,size),'big')
    def put(u,address,value,size=2):u.mem_write(address,value.to_bytes(size,'big'))
    def call(kind,name,args=()):
        u,sym=contexts[kind];stack=0xffdf00 if kind=='main' else 0x1ff00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((n&0xffffffff).to_bytes(4,'big') for n in args))
        try:u.emu_start(sym[name],stop,count=4000000)
        except Exception as error:raise AssertionError(f'{kind} {name} at {u.reg_read(UC_M68K_REG_PC):x}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed C stack'
        return u.reg_read(UC_M68K_REG_D0)
    cases=[(tool.encode(raw),raw) for raw in vectors()]
    for name in ['garden','cavern','noise']:
        cases.append(((project/'disc'/(name+'.kos')).read_bytes(),(project/'disc'/(name+'.mda')).read_bytes()))
    # Exercise short-offset and no-output continuation tokens too; the authoring
    # encoder chooses long references, but the SDK grammar accepts both.
    w=tool.Writer();w.bit(1);w.payload(65)
    for b in (0,0,0,1):w.bit(b)
    w.payload(255);w.bit(0);w.bit(1);w.payload(0,0,1)
    w.bit(0);w.bit(1);w.payload(0,0,0);cases.append((bytes(w.data),b'AAAA'))
    for packed,raw in cases:
        sub.mem_write(0x80000,packed);sub.mem_write(0x8fffc,b'GUAR'+b'\xa5'*len(raw)+b'TAIL')
        assert call('sub','cp_kos_valid',(0x80000,len(packed),len(raw)))==1
        assert not call('sub','cp_kos_valid',(0x80000,len(packed)-1,len(raw)))
        assert not call('sub','cp_kos_valid',(0x80000,len(packed),len(raw)-1))
        saved={getattr(mc,'UC_M68K_REG_'+n):0x13570000+i for i,n in enumerate(['D2','D3','D4','D5','D6','D7','A2','A3','A4','A5','A6'])}
        for reg,value in saved.items():sub.reg_write(reg,value)
        call('sub','cp_kos_decode',(0x80000,0x90000))
        assert all(sub.reg_read(reg)==value for reg,value in saved.items()),'SDK adapter broke the C ABI'
        assert bytes(sub.mem_read(0x90000,len(raw)))==raw,'SDK Kosinski bytes differ from original'
        assert bytes(sub.mem_read(0x8fffc,4))==b'GUAR' and bytes(sub.mem_read(0x90000+len(raw),4))==b'TAIL','Decoder escaped proven output bounds'
        sub.mem_write(0x80000,raw);sub.mem_write(0x90000,b'\xa5'*len(raw))
        call('sub','cp_raw_copy',(0x80000,0x90000,len(raw)))
        assert bytes(sub.mem_read(0x90000,len(raw)))==raw
        assert bytes(sub.mem_read(0x90000+len(raw),4))==b'TAIL'
    for before,after,blanks,rate,wanted in [(4090,5,0,hz,11),(3000,904,4,60,2000),
                                         (0,404,8,60,4500),(0,520,32,50,21000),
                                         (0,1,5,60,65535),(0,0,33,50,65535)]:
        assert call('sub','cp_elapsed',(before,after,blanks,rate))&65535==wanted
    files={p.name.upper()+';1':p.read_bytes() for p in (project/'disc').iterdir() if p.suffix in ('.mda','.kos')}
    names=sorted(files)
    entries=b''.join(name.encode().ljust(14,b'\0')+(40+i*16).to_bytes(4,'big')+len(files[name]).to_bytes(4,'big') for i,name in enumerate(names))
    sub.mem_write(boot['dir_cache'],entries);put(sub,boot['dir_entry_count'],len(names))
    for n in ['access_op','access_op_result']:put(sub,ss[n],0)
    for n in ['filename','filebuff','filesize']:put(sub,ss[n],0,4)
    mailbox=bytearray(0x30);owner='main';physical=bytearray(262144);stopwatch=4080
    def install(u,kind,base):
        def rd(cpu,access,address,size,value,user):
            nonlocal stopwatch
            if address==0xff800c:
                u.mem_write(address,stopwatch.to_bytes(2,'big'));stopwatch=(stopwatch+37)&4095;return
            if base<=address<base+0x30:
                off=address-base;mailbox[2:4]=(1 if owner=='main' else 2).to_bytes(2,'big')
                u.mem_write(address,bytes(mailbox[off:off+size]))
            start=0x200000 if kind=='main' else 0x80000
            if start<=address<start+262144:
                assert owner==kind,'Read from foreign Word RAM';off=address-start
                if kind=='main':u.mem_write(address,bytes(physical[off:off+size]))
        def wr(cpu,access,address,size,value,user):
            nonlocal owner
            value&=(1<<(size*8))-1
            if address==0xff800c:raise AssertionError('Stopwatch reset would interfere with the CDC owner')
            if base<=address<base+0x30:
                off=address-base
                if off==3:
                    if kind=='main' and value&2:owner='sub'
                    elif kind=='sub' and value&1:owner='main'
                elif off>=0x10:
                    assert (kind=='main' and off<0x20) or (kind=='sub' and off>=0x20)
                    mailbox[off:off+size]=value.to_bytes(size,'big')
                    if off==0x20 and value:
                        assert owner=='main' and int.from_bytes(mailbox[0x2c:0x2e],'big')==value
            start=0x200000 if kind=='main' else 0x80000
            if start<=address<start+262144:
                assert owner==kind,'Write into foreign Word RAM';off=address-start
                if kind=='sub':assert 65536<=off<98304,'Decoder wrote outside output reservation'
                physical[off:off+size]=value.to_bytes(size,'big')
        u.hook_add(UC_HOOK_MEM_READ,rd);u.hook_add(UC_HOOK_MEM_WRITE,wr)
    install(main,'main',0xa12000);install(sub,'sub',0xff8000)
    now=0;pending=None;delay=5;failure=False;bad_size=False;corrupt=False;uploads=[]
    def frame(upload=True):
        nonlocal now,pending,owner
        now=(now+1)&65535;mailbox[0x26:0x28]=now.to_bytes(2,'big');mailbox[0x16:0x18]=now.to_bytes(2,'big')
        if get(sub,ss['access_op'])==3:
            if pending is None:
                name=bytes(sub.mem_read(get(sub,ss['filename'],4),14)).split(b'\0')[0].decode()
                assert get(sub,ss['filebuff'],4)==0x80000 and owner=='sub';pending=[delay,name]
            assert owner=='sub' and get(sub,ss['filebuff'],4)==0x80000
            pending[0]-=1
            if pending[0]<=0:
                data=bytearray(files[pending[1]])
                if corrupt=='crc':data[100]^=1
                elif corrupt:data[:2]=b'\x00\x00'
                physical[:len(data)]=data
                sub.mem_write(0x80000,bytes(data))
                put(sub,ss['filesize'],len(data)+(1 if bad_size else 0),4)
                put(sub,ss['access_op_result'],0xff9c if failure else 0x64);put(sub,ss['access_op'],0);pending=None
        call('sub','stream_sub_tick',(now,hz));call('main','stream_main_tick',(300//hz,))
        if upload and get(main,ms['stream_phase'])==5:
            offset=get(main,ms['stream_uploaded']);count=min(256,get(main,ms['stream_cache_tiles'])*32-offset)
            assert owner=='main' and not get(main,ms['stream_ready'])
            expected=bytes(physical[65536+64+offset:65536+64+offset+count])
            call('main','stream_prepare')
            frame_base=ms['frames']+(get(main,ms['published'])-1)*1050
            assert get(main,frame_base+946)*2==count
            assert bytes(main.mem_read(frame_base+624,count))==expected,'Renderer copied encoded input instead of decoded output'
            # The VBlank upload consumes a resident copy, never a pointer into
            # Word RAM that might already belong to Sub again.
            owner='sub';ports.clear();call('main','vblank_user');owner='main'
            assert b''.join(ports[-count//2:])==expected,'Published pattern bytes differ'
            put(main,ms['upload_ack'],0)
            uploads.append(expected);call('main','stream_upload_done',(count,))
    def finish():
        for _ in range(2000):
            frame()
            if not get(main,ms['stream_phase']):return
        raise AssertionError('Reader did not retire')
    for asset in range(6):
        uploads.clear();assert call('main','stream_start',(asset,))==1
        assert not call('main','stream_start',(asset^1,));finish()
        name=['garden','cavern','noise'][asset//2];raw=(project/'disc'/(name+'.mda')).read_bytes()
        assert get(main,ms['stream_ready'])==1 and get(main,ms['stream_decode_ticks'])==37,(asset,get(main,ms['stream_result']),get(main,ms['stream_errors']),get(main,ms['stream_decode_ticks']),get(main,ms['stream_read_frames']))
        assert get(main,ms['stream_read_frames'])==delay
        assert bytes(physical[65536:65536+len(raw)])==raw
        assert b''.join(uploads)==raw[64:-1600]
        assert bytes(main.mem_read(ms['stream_cache_map'],1600))==raw[-1600:]
    delay=40;assert call('main','stream_start',(1,))==1
    for _ in range(12):frame()
    assert get(sub,ss['access_op'])==3;call('main','stream_cancel')
    assert not call('main','stream_start',(0,));finish()
    assert not get(main,ms['stream_ready']) and get(main,ms['stream_result'])==2 and owner=='main'
    delay=5
    assert call('main','stream_start',(6,));finish();assert get(main,ms['stream_result'])==3
    # Refuse rounded/file length before the reader sees a destination.
    address=boot['dir_cache']+names.index('GARDEN.KOS;1')*22+18
    put(sub,address,32769,4);assert call('main','stream_start',(1,));finish()
    assert get(main,ms['stream_result'])==4;put(sub,address,len(files['GARDEN.KOS;1']),4)
    for defect,result in [('io',5),('size',4),('format',6),('crc',6)]:
        failure=defect=='io';bad_size=defect=='size';corrupt='crc' if defect=='crc' else defect=='format'
        assert call('main','stream_start',(0 if defect=='crc' else 1,));finish()
        assert get(main,ms['stream_result'])==result and not get(main,ms['stream_ready'])
    failure=bad_size=corrupt=False
    # Deadline retains ownership and the token until the old CDC read retires.
    delay=hz*31;assert call('main','stream_start',(1,))
    for _ in range(hz*30+5):frame()
    assert get(main,ms['stream_phase'])==6 and owner=='sub'
    assert not call('main','stream_start',(0,));finish()
    assert owner=='main' and get(main,ms['stream_result'])==7
    delay=5;assert call('main','stream_start',(1,));finish();assert get(main,ms['stream_ready'])
    return True

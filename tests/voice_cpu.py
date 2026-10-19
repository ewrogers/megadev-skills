"""Compiled M68000 voice adapters, modeled CDC completion and RF5C164 addressing.

Not a seek-time, bus-contention, sound-quality or physical-console measurement.
"""
from startup import check,symbols

def check_voice(project,hz):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_HOOK_CODE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0,UC_M68K_REG_A0
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    boot=symbols((project/'build/sp.bin.elf.sym').read_text())
    sub.mem_write(0x6000,(project/'build/sp.bin').read_bytes())
    for u,sym in contexts.values():u.mem_map(0x300000,0x1000)
    def get(u,address,size=2):return int.from_bytes(u.mem_read(address,size),'big')
    def put(u,address,value,size=2):u.mem_write(address,value.to_bytes(size,'big'))
    files={p.name.upper()+';1':p.read_bytes() for p in (project/'disc').glob('*.pcm')}
    names=sorted(files)
    entries=b''.join(name.encode().ljust(14,b'\0')+(40+i*8).to_bytes(4,'big')+len(files[name]).to_bytes(4,'big') for i,name in enumerate(names))
    sub.mem_write(boot['dir_cache'],entries);put(sub,boot['dir_entry_count'],len(names))
    for name in ('access_op','access_op_result'):put(sub,ss[name],0)
    for name in ('filename','filebuff','filesize'):put(sub,ss[name],0,4)
    mailbox=bytearray(0x30);pcm=bytearray(65536);regs=[bytearray(7) for _ in range(8)]
    bank=channel=0;disabled=255;position=0;now=0;clock_remainder=0;initializing=True
    pcm_writes=0;frame_writes=0;max_writes=0;busy=0;commands=[];read=None;delay=6
    hold=False;bad_result=False;short=False;corrupt=None;tear=False;heard=[];last_region=None
    v=ss['voice_stream'];field=lambda i:get(sub,v+i*2)
    def install(u,kind,base):
        def rd(cpu,access,address,size,value,user):
            if base<=address<base+0x30:
                offset=address-base;u.mem_write(address,bytes(mailbox[offset:offset+size]))
            if kind=='sub' and address in (0xff002d,0xff002f):
                value=(position>>11)>>(8 if address==0xff002f else 0)&255
                if tear and address==0xff002f:value=(value+(rd.flip&1))&255;rd.flip+=1
                u.mem_write(address,bytes([value]))
        rd.flip=0
        def wr(cpu,access,address,size,value,user):
            nonlocal bank,channel,disabled,position,pcm_writes,frame_writes
            value&=(1<<(size*8))-1
            if base<=address<base+0x30:
                offset=address-base
                assert (kind=='main' and 0x10<=offset<0x20) or (kind=='sub' and offset>=0x20)
                mailbox[offset:offset+size]=value.to_bytes(size,'big')
            if kind!='sub':return
            if address==0xff000f:
                assert value&128
                if value&64:channel=value&7
                else:bank=value&15
            elif address==0xff0011:
                disabled=value
                if disabled&8:position=regs[3][6]<<19
            elif 0xff0001<=address<=0xff000d and address&1:
                regs[channel][(address-0xff0001)//2]=value
                if channel==3 and address==0xff000d and disabled&8:position=value<<19
            elif 0xff2001<=address<=0xff3fff:
                assert size==1 and address&1
                offset=bank*4096+(address-0xff2001)//2
                if not initializing:
                    assert 0x4000<=offset<0xc000,'Streaming damaged resident or unreserved PCM RAM'
                    pos=position>>11
                    assert disabled&8 or not (pos//16384==offset//16384),'Upload overwrote a playing half'
                    assert field(4)!=((offset-0x4000)//16384),'Policy released a live half'
                    frame_writes+=1
                pcm[offset]=value;pcm_writes+=1
        u.hook_add(UC_HOOK_MEM_READ,rd);u.hook_add(UC_HOOK_MEM_WRITE,wr)
    install(main,'main',0xa12000);install(sub,'sub',0xff8000)
    def bios(u,address,size,user):
        nonlocal busy
        op=u.reg_read(UC_M68K_REG_D0)&65535;sr=u.reg_read(UC_M68K_REG_SR)
        if op==0x80:u.reg_write(UC_M68K_REG_SR,(sr|1) if busy else sr&~1)
        else:
            assert not busy and not get(sub,ss['access_op']),'Music overlapped CDC'
            commands.append((op,get(sub,u.reg_read(UC_M68K_REG_A0))));busy=3
        stack=u.reg_read(UC_M68K_REG_A7);u.reg_write(UC_M68K_REG_PC,get(u,stack,4));u.reg_write(UC_M68K_REG_A7,stack+4)
    sub.hook_add(UC_HOOK_CODE,bios,begin=0x5f22,end=0x5f22)
    def call(kind,name,args=()):
        u,sym=contexts[kind];stack=0xffdf00 if kind=='main' else 0x1ff00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((n&0xffffffff).to_bytes(4,'big') for n in args))
        try:u.emu_start(sym[name],stop,count=1000000)
        except Exception as error:raise AssertionError(f'{kind} {name} at {u.reg_read(UC_M68K_REG_PC):x}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4
        return u.reg_read(UC_M68K_REG_D0)
    call('sub','audio_init');initializing=False
    assert pcm_writes==16390
    resident=bytes(pcm[:0x4000])
    def frame():
        nonlocal now,busy,read,position,clock_remainder,last_region,frame_writes,max_writes
        now=(now+1)&65535;frame_writes=0
        if busy:busy-=1
        if get(sub,ss['access_op'])==3:
            if read is None:
                pointer=get(sub,ss['filename'],4);name=bytes(sub.mem_read(pointer,14)).split(b'\0')[0].decode()
                destination=get(sub,ss['filebuff'],4)
                assert name in files and not busy and get(sub,ss['audio_state']+8)==1
                assert destination==ss['staging'] and destination+16384<0x2c000
                read=[delay,name,destination]
            assert get(sub,ss['filebuff'],4)==read[2] and get(sub,ss['access_op'])==3,'Live CDC destination reused'
            if not hold:
                read[0]-=1
                if read[0]<=0:
                    data=bytearray(files[read[1]])
                    if corrupt=='marker':data[40]=255
                    if corrupt=='sum':data[40]^=1
                    sub.mem_write(read[2],bytes(data))
                    put(sub,ss['filesize'],16383 if short else 16384,4)
                    put(sub,ss['access_op_result'],0xff9c if bad_result else 0x64);put(sub,ss['access_op'],0);read=None
        # RF5C164 clock/marker semantics from ares' PCM implementation. This
        # is a behavioral oracle, not an emulation of CDC or bus timings.
        clock_remainder+=12500000
        clocks,clock_remainder=divmod(clock_remainder,384*hz)
        if not disabled&8:
            step=regs[3][2]|regs[3][3]<<8;loop=regs[3][4]|regs[3][5]<<8
            assert step==503,'PAL/NTSC changed PCM pitch'
            for _ in range(clocks):
                pos=position>>11
                if pcm[pos]==255:position=loop<<11;pos=loop
                else:position=(position+step)&0x7ffffff
                region=0 if 0x4000<=pos<0x8000 else 1 if 0x8000<=pos<0xc000 else 2
                if region!=last_region and region<2:
                    name=('L' if get(sub,ss['source']) else 'V')+f'{len(heard):03}.PCM;1'
                    assert bytes(pcm[0x4000+region*16384:0x8000+region*16384])==files[name],('Partial/stale audio',name,region)
                    heard.append(name)
                last_region=region
        else:last_region=None
        call('sub','audio_sub_tick',(now,hz));call('sub','voice_sub_tick',(now,hz))
        # The tight foreground loop must not upload another allowance until a
        # distinct service epoch arrives.
        call('sub','voice_sub_tick',(now,hz))
        call('main','audio_main_tick',(300//hz,))
        max_writes=max(max_writes,frame_writes);assert frame_writes<=512
        assert bytes(pcm[:0x4000])==resident,'Streaming replaced resident samples'
    def advance(n=8):
        for _ in range(n):frame()
    def send(op,arg=0):
        assert call('main','audio_request',(op,arg))==1
        advance(4);assert not get(main,ms['audio_pending'])
    def until(predicate,n=hz*50):
        for _ in range(n):
            if predicate():return
            frame()
        raise AssertionError(('Voice state did not arrive',field(0),field(2),field(3),heard))
    def start(arg=0):
        nonlocal heard,last_region
        heard=[];last_region=None;send(10,arg)
    def stop():
        send(11);until(lambda:field(0)==0)
    send(1,2);advance();assert commands[-1]==(0x13,2)
    start();until(lambda:field(0)==2)
    assert (2,2) in commands,'CD music was not stopped before streaming'
    # A scenery request may arrive during a voice, but cannot release its lease
    # or publish a reply until the voice retires. Word RAM ownership alone does
    # not authorize a second optical-drive client.
    mailbox[2:4]=b'\x00\x02';mailbox[0x10:0x12]=b'\x00\x05'
    call('sub','stream_sub_tick',(now,hz))
    assert get(sub,ss['audio_state']+8)==1 and not int.from_bytes(mailbox[0x20:0x22],'big')
    mailbox[0x10:0x12]=bytes(2)
    send(1,3);send(4,0);assert not disabled&2 and not disabled&8,'Resident effect or voice was silenced'
    until(lambda:field(0)==4)
    assert heard==sorted(n for n in files if n.startswith('V')) and field(3)==len(heard)
    assert not field(9),'Normal buffering underruns'
    advance();assert commands[-1]==(0x13,3),'Latest CD intent did not resume'
    start(1);until(lambda:field(0)==4)
    assert heard==sorted(n for n in files if n.startswith('L')) and not field(9)
    # Longer nonvoice sample, deliberately too little upload bandwidth.
    start(3);until(lambda:field(0)==2);until(lambda:field(9)>0)
    assert not disabled&8,'Underrun must reach the hardware silence loop, not stop unrelated channels'
    until(lambda:field(0)==2);advance(2)
    assert len(heard)>=3,'Rebuffer did not resume the next unplayed block'
    stop();assert disabled&8
    # Even cancellation before CD-DA has finished stopping releases the lease
    # only through the owner; a silence request cannot resurrect stale music.
    send(1,2);advance();assert commands[-1]==(0x13,2)
    assert call('main','audio_request',(10,0))==1;advance(2)
    assert busy and get(sub,ss['audio_state']+8)==1 and not get(sub,ss['access_op'])
    send(7);advance();assert get(sub,ss['audio_state']+8)==0 and field(0)==0
    before=len(commands);advance(10);assert len(commands)==before
    send(1,3);advance()
    # Cancel during an unretired CDC request: reject new work and hold ownership.
    hold=True;start();until(lambda:get(sub,ss['access_op'])==3)
    destination=get(sub,ss['filebuff'],4);send(11);assert field(0)==3
    previous=field(10);send(10,1);assert field(10)==previous+1
    assert get(sub,ss['filebuff'],4)==destination and get(sub,ss['audio_state']+8)==1
    hold=False;until(lambda:field(0)==0);advance();assert commands[-1]==(0x13,3)
    # Directory length is checked BEFORE CDC gets a writable buffer.
    address=boot['dir_cache']+names.index('V000.PCM;1')*22+18
    put(sub,address,16385,4);start();until(lambda:field(0)==5)
    assert not get(sub,ss['access_op']);put(sub,address,16384,4)
    entry=address-18;sub.mem_write(entry,b'X');start();until(lambda:field(0)==5)
    assert not get(sub,ss['access_op']);sub.mem_write(entry,b'V')
    for defect in ('io','short','marker','sum'):
        bad_result=defect=='io';short=defect=='short';corrupt=defect
        start();until(lambda:field(0)==5);assert disabled&8 and not get(sub,ss['audio_state']+8)
    bad_result=short=False;corrupt=None
    # A stalled worker stops before refilling; it does not pretend old playback
    # can be reconstructed after an unbounded CPU scheduling gap.
    start();until(lambda:field(0)==2)
    now=(now+hz+1)&65535;call('sub','voice_sub_tick',(now,hz));assert disabled&8
    until(lambda:field(0)==5)
    # Torn hardware pointer reads have a bounded retry and fail closed.
    start();until(lambda:field(0)==2);tear=True;frame();tear=False
    until(lambda:field(0)==5);assert disabled&8
    # A timed-out read remains quarantined until it actually retires.
    hold=True;start();until(lambda:get(sub,ss['access_op'])==3)
    advance(hz*30+5);assert field(0)==3 and get(sub,ss['audio_state']+8)==1
    hold=False;until(lambda:field(0)==5);assert not get(sub,ss['audio_state']+8)
    # Rejected wire arguments do not alter mailbox parameters.
    for op,arg in [(8,0),(10,4),(11,1)]:
        before=bytes(mailbox);assert not call('main','audio_request',(op,arg));assert bytes(mailbox)==before
    mailbox[0x10:0x12]=b'\x00\x05'
    assert not call('main','audio_request',(10,0)),'Voice start ignored a pending scenery request'
    previous=field(10);call('sub','voice_request',(0,0));assert field(10)==previous+1
    mailbox[0x10:0x12]=bytes(2)
    assert max_writes==512
    return True

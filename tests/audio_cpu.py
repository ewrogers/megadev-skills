"""Execute original-68000 audio adapters with modeled BIOS completion and PCM MMIO."""
from startup import check,symbols

def check_audio(project,hz,dialogue=False):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_HOOK_CODE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0,UC_M68K_REG_A0
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    for u,sym in contexts.values():u.mem_map(0x300000,0x1000)
    mailbox=bytearray(0x30);pcm=bytearray(65536);bank=0;channel=0;disabled=0xff
    registers=[bytearray(7) for _ in range(8)];pcm_writes=0;configured=[]
    busy=0;forever=False;reader_live=False;commands=[]
    def word(u,address,size=2):return int.from_bytes(u.mem_read(address,size),'big')
    def put(u,address,value,size=2):u.mem_write(address,value.to_bytes(size,'big'))
    def install(u,kind,base):
        def read(cpu,access,address,size,value,user):
            if base<=address<base+0x30:
                off=address-base;u.mem_write(address,bytes(mailbox[off:off+size]))
        def write(cpu,access,address,size,value,user):
            nonlocal bank,channel,disabled,pcm_writes
            value&=(1<<(size*8))-1
            if base<=address<base+0x30:
                off=address-base
                assert (kind=='main' and 0x10<=off<0x20) or (kind=='sub' and off>=0x20),'Wrong mailbox writer'
                mailbox[off:off+size]=value.to_bytes(size,'big')
            if kind=='sub':
                if address==0xFF000F:
                    assert value&0x80,'PCM globally disabled during register selection'
                    if value&0x40:channel=value&7
                    else:bank=value&15
                elif address==0xFF0011:disabled=value
                elif 0xFF0001<=address<=0xFF000D and address&1:
                    registers[channel][(address-0xFF0001)//2]=value
                    if address==0xFF000D:configured.append((channel,bytes(registers[channel])))
                elif 0xFF2001<=address<=0xFF3FFF:
                    assert address&1 and size==1,'PCM RAM byte lane wrong'
                    assert disabled==0xff,'Sample RAM replaced while voices are active'
                    pcm[bank*4096+(address-0xFF2001)//2]=value;pcm_writes+=1
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    install(main,'main',0xA12000);install(sub,'sub',0xFF8000)
    def bios(u,address,size,user):
        nonlocal busy
        op=u.reg_read(UC_M68K_REG_D0)&65535
        sr=u.reg_read(UC_M68K_REG_SR)
        if op==0x80:u.reg_write(UC_M68K_REG_SR,(sr|1) if busy or forever else (sr&~1))
        else:
            assert not busy,'BIOS command overwritten before completion'
            assert not reader_live,'CD music command issued while the data reader was live'
            assert op in [2,3,4,0x13],op
            track=word(u,u.reg_read(UC_M68K_REG_A0))
            if op==0x13:assert track in [2,3],'Track ABI pointer does not contain a track word'
            commands.append((op,track));busy=3
        stack=u.reg_read(UC_M68K_REG_A7);u.reg_write(UC_M68K_REG_PC,word(u,stack,4));u.reg_write(UC_M68K_REG_A7,stack+4)
    sub.hook_add(UC_HOOK_CODE,bios,begin=0x5F22,end=0x5F22)
    def call(kind,name,args=()):
        u,sym=contexts[kind];stack=0xFFDF00 if kind=='main' else 0x1FF00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
        try:u.emu_start(sym[name],stop,count=3000000 if dialogue else 1000000)
        except Exception as error:raise AssertionError(f'{kind} {name}: PC {u.reg_read(UC_M68K_REG_PC):x}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' changed stack'
        return u.reg_read(UC_M68K_REG_D0)
    call('sub','audio_init')
    assert pcm_writes==(49158 if dialogue else 16390) and disabled==0xff
    for index,name in enumerate(['bed','bell','pluck','drum']):
        data=(project/'res'/f'{name}.pcm').read_bytes()
        assert bytes(pcm[index*4096:index*4096+len(data)])==data
        assert pcm[index*4096+len(data)]==0xff and 0xff not in data
    assert pcm[0x3f00:0x3f02]==b'\x80\xff','One-shot silence loop missing'
    if dialogue:
        for base,name in [(0x4000,'welcome'),(0x8000,'listen')]:
            data=(project/'res'/f'voice_{name}.pcm').read_bytes()
            assert 1<=len(data)<=12000 and 0xff not in data
            assert bytes(pcm[base:base+len(data)])==data and pcm[base+len(data)]==0xff
    now=0
    def frame():
        nonlocal now,busy
        now=(now+1)&65535
        if busy:busy-=1
        call('sub','audio_sub_tick',(now,hz));call('main','audio_main_tick',(300//hz,))
    def advance(frames=8):
        for i in range(frames):frame()
    def send(op,arg=0):
        assert call('main','audio_request',(op,arg))==1
        before=bytes(mailbox);assert not call('main','audio_request',(op,arg));assert bytes(mailbox)==before
        advance(4);assert not word(main,ms['audio_pending'])
    state=ss['audio_state'];field=lambda index:word(sub,state+index*2)
    send(1,2);advance();assert field(2)==2 and commands[-1][0]==0x13
    send(5);assert not disabled&1 and registers[0][:4]==bytes([0x48,0x88,0xf7,1])
    send(4,0);send(4,1);assert disabled&6==0
    send(4,2);before=len(configured);send(4,0);assert len(configured)==before,'Low-priority effect stole protected voice'
    advance(hz);assert disabled&6==6 and not disabled&1,'One-shots did not release independently of bed'
    send(6,0);assert registers[0][1]==0x0c
    send(6,2);assert registers[0][1]==0xc0
    send(2);advance();assert field(3)==1 and commands[-1][0]==3
    send(2);advance();assert field(3)==0 and commands[-1][0]==4
    assert not call('sub','audio_drive_acquire')
    advance();assert call('sub','audio_drive_acquire')==1 and commands[-1][0]==2
    reader_live=True;before=len(commands)
    send(1,3);send(4,0);advance(20)
    assert len(commands)==before and field(0)==3 and not disabled&1,'Data load lost music intent/PCM residency'
    reader_live=False;call('sub','audio_drive_release');advance()
    assert field(2)==3 and commands[-1]==(0x13,3),'Music did not restart after releasing drive'
    send(7);advance();assert disabled==0xff and not field(0) and not field(7) and not field(2)
    before=(len(commands),bytes(mailbox));assert not call('main','audio_request',(1,1));assert before==(len(commands),bytes(mailbox)),'Invalid track changed the request'
    if dialogue:
        for op,arg in [(8,0),(8,3),(9,1)]:
            before=bytes(mailbox);assert not call('main','audio_request',(op,arg));assert bytes(mailbox)==before
        model=ms['dialogue']
        call('main','d_init',(model,));assert call('main','d_start',(model,0))
        call('main','d_present',(model,word(main,model+1080)))
        call('main','dialogue_audio_tick',(5,0))
        assert word(main,ms['audio_pending'])==1
        # Cancel while a start is committed but before Sub has consumed it.
        call('main','d_step',(model,16,0,0));call('main','dialogue_audio_tick',(5,0))
        advance(4);assert not disabled&8,'Delayed start did not exercise the stale-result case'
        assert bytes(registers[3])==bytes([0x90,0x88,0xf7,1,0,0x3f,0x40])
        call('main','dialogue_audio_tick',(5,0))
        assert word(main,ms['audio_pending'])==1,'Stale start was not followed by an ordered stop'
        advance(4);call('main','dialogue_audio_tick',(5,0));assert disabled&8 and not word(main,ms['speech']+4)
        call('main','d_start',(model,1));call('main','d_present',(model,word(main,model+1080)))
        call('main','dialogue_audio_tick',(5,0));advance(4);call('main','dialogue_audio_tick',(5,0))
        assert not disabled&8 and registers[3][6]==0x80
        send(4,1);send(9);assert disabled&8 and disabled&6!=6,'Speech stop also silenced an independent effect'
        send(8,2);advance(hz*2+5);assert disabled&8,'Voice did not stop at its finite lifetime'
        assert bytes(pcm[0x4000:0x4000+len((project/'res/voice_welcome.pcm').read_bytes())])==(project/'res/voice_welcome.pcm').read_bytes()
    # A BIOS command that never completes quarantines drive ownership; PCM remains serviceable.
    forever=True;send(1,2);advance(hz*30+5)
    assert field(11)==1 and not call('sub','audio_drive_acquire')
    before=len(commands);send(4,2);assert len(commands)==before and disabled!=0xff
    assert (int.from_bytes(mailbox[0x28:0x2a],'big')>>8)==5
    # A lost Sub acknowledgement does not silently free the command slot.
    assert call('main','audio_request',(4,0))==1
    for i in range(hz*30+1):call('main','audio_main_tick',(300//hz,))
    assert word(main,ms['audio_pending'])==3 and word(main,ms['audio_link_fault'])==1
    assert not call('main','audio_request',(4,1))
    if dialogue:
        call('main','dialogue_audio_tick',(5,0))
        assert word(main,ms['speech']+16)==1,'Lost owner acknowledgement did not make voice unavailable'
    return True

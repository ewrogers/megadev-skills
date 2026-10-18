"""Execute replay/region checks and the integration coordinator on original 68000."""
from startup import check,symbols
from streaming_cpu import check_streaming

def check_testing(project,hz):
    from unicorn import UC_HOOK_CODE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    check_streaming(project,hz,entry_controls=False,with_audio=True)
    table=symbols((project/'build/ipx.mmd.elf.sym').read_text());saved=[]
    def collect(u,sym):saved.append((u,sym));return True
    check((project/'disc/ipx.mmd').read_bytes(),table,'main',collect)
    u,sy=saved[0];u.mem_map(0x300000,0x1000)
    def get(address,size=2):return int.from_bytes(u.mem_read(address,size),'big')
    def put(address,value,size=2):u.mem_write(address,value.to_bytes(size,'big'))
    def call(name,args=()):
        stack=0xFFDF00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join(v.to_bytes(4,'big') for v in args))
        try:u.emu_start(sy[name],stop,count=1500000)
        except Exception as e:raise AssertionError(name+' PC='+hex(u.reg_read(UC_M68K_REG_PC))) from e
        assert u.reg_read(UC_M68K_REG_PC)==stop and u.reg_read(UC_M68K_REG_A7)==stack+4,name+' did not return cleanly'
        return u.reg_read(UC_M68K_REG_D0)
    rp=sy['replay'];state=rp;initial=rp+20;tape=rp+40
    # Struct: two 20-byte states + 720*8 samples + two 32-bit hashes + eight words.
    length=rp+5808;cursor=length+2;mode=length+4;first_bad=length+6
    call('rp_init',(rp,0xACE1));call('rp_record',(rp,))
    for i in range(240):call('rp_tick',(rp,8,1 if i==180 else 0,0))
    call('rp_stop',(rp,));assert get(length)==240
    expected=call('rp_hash',(state,));assert expected==0xa61a03c9
    before=bytes(u.mem_read(tape,240*8));assert call('rp_play',(rp,))==1
    for i in range(hz*4):assert call('rp_advance',(rp,300//hz,0,0))==1
    assert get(mode)==3 and call('rp_hash',(state,))==expected
    assert bytes(u.mem_read(tape,240*8))==before,'Replay modified the input tape'
    assert call('rp_play',(rp,))==1;call('rp_tick',(rp,0,0,1))
    assert get(mode)==4 and get(first_bad)==0 and get(cursor)==0
    assert call('rp_play',(rp,))==1
    put(tape+180*8+2,0)
    for i in range(240):call('rp_tick',(rp,0,0,0))
    assert get(mode)==4 and get(first_bad)==180
    put(tape+180*8+2,1)
    # Full buffer stops exactly at capacity, and no terminal tick changes the snapshot.
    call('rp_record',(rp,))
    for i in range(725):call('rp_tick',(rp,8,0,0))
    assert get(length)==720 and get(mode)==5
    frozen=bytes(u.mem_read(rp,5824));call('rp_tick',(rp,0,0,1));assert bytes(u.mem_read(rp,5824))==frozen
    # Entire invalid tape is refused before resetting gameplay.
    put(tape+719*8,0x8000);snapshot=bytes(u.mem_read(state,20))
    assert call('rp_play',(rp,))==0 and bytes(u.mem_read(state,20))==snapshot
    # Coordinator is composed with existing services; control completions without fake drive timing claims.
    calls=[];accept=True;auto_complete=False
    def stub(u,address,size,user):
        nonlocal accept
        if address not in (sy['audio_request'],sy['stream_start']):return
        stack=u.reg_read(UC_M68K_REG_A7);arg=get(stack+4,4);extra=get(stack+8,4) if address==sy['audio_request'] else 0
        calls.append(('audio' if address==sy['audio_request'] else 'stream',arg,extra))
        if accept and auto_complete and address==sy['stream_start']:
            put(sy['stream_result'],3 if arg==2 else 1)
            put(sy['stream_ready'],0 if arg==2 else 1)
            if arg!=2:
                put(sy['stream_pass'],get(sy['stream_pass'])+1);put(sy['active'],arg)
        u.reg_write(UC_M68K_REG_D0,1 if accept else 0)
        u.reg_write(UC_M68K_REG_PC,get(stack,4));u.reg_write(UC_M68K_REG_A7,stack+4)
    u.hook_add(UC_HOOK_CODE,stub)
    for name,value in [('hz',hz),('main_ticks',100),('suite_due',100),('suite',1),('active',0),('transition',0),('stream_phase',0),('audio_pending',0),('stream_errors',0),('audio_link_fault',0)]:put(sy[name],value)
    put(0xA12028,0x100)
    accept=False;call('suite_tick');assert get(sy['suite'])==1
    accept=True;call('suite_tick');assert get(sy['suite'])==2 and calls[-1]==('audio',1,2)
    call('suite_tick');assert get(sy['suite'])==3 and calls[-1]==('stream',1,0)
    count=len(calls);put(sy['stream_phase'],2);call('suite_tick');assert len(calls)==count
    put(sy['stream_phase'],0);put(sy['stream_pass'],1);call('suite_tick');assert get(sy['suite'])==4 and calls[-1]==('audio',1,3)
    call('suite_tick');assert get(sy['suite'])==5 and calls[-1]==('stream',2,0)
    put(sy['stream_result'],3);call('suite_tick');assert get(sy['suite'])==6 and calls[-1]==('stream',1,0)
    put(sy['stream_ready'],1);put(sy['stream_result'],1);call('suite_tick')
    assert get(sy['suite'])==7 and get(sy['suite_pass'])==3
    put(sy['suite'],1);put(sy['main_ticks'],100+hz*40+1);call('suite_tick')
    assert get(sy['suite'])==8 and get(sy['suite_fail'])==1
    if 'testing_tour' in sy:
        for name in ['suite','suite_due','suite_pass','suite_fail','clock_fault','video_fault','main_ticks','stream_phase','stream_pass','audio_pending','transition','active']:
            put(sy[name],0)
        auto_complete=True;saw_record=False;last_length=0
        for frame in range(hz*20):
            put(sy['main_ticks'],frame)
            call('suite_tick');call('testing_tour',(1,))
            current_length=get(length)
            assert current_length-last_length<=2,'Automatic recorder blocked for a whole tape'
            last_length=current_length
            if get(mode)==1:saw_record=True
            if get(sy['tour_ok']):break
        else:raise AssertionError('Automatic test tour did not pass')
        assert saw_record and frame>=hz*11 and get(mode)==3 and get(length)==240
        put(sy['clock_fault'],1);call('testing_tour',(1,));assert not get(sy['tour_ok']),'Overloaded tour reported PASS'
    return True

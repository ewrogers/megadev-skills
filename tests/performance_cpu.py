"""Original-68000 work/observer checks. Stopwatch values are injected, not cycle measurements."""
from startup import check,symbols
from scaling_cpu import check_scaling

def oracle(phase,count,repeats):
    result=0
    for _ in range(repeats):
        value=0x12345678^phase
        for i in range(count):
            index=(phase+i*13)%300;value=(value+((index^0x5a5a)+(index<<3)))&0xffffffff
        result^=value;result=((result<<1)|(result>>31))&0xffffffff
        phase=(phase+7)%300
    return result

def check_performance(project,hz):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    check_scaling(project,hz,performance=True)
    table=symbols((project/'build/ipx.mmd.elf.sym').read_text());contexts=[]
    def collect(u,sy):contexts.append((u,sy));return True
    check((project/'disc/ipx.mmd').read_bytes(),table,'main',collect)
    u,sy=contexts[0];u.mem_map(0x300000,0x1000)
    def put(address,value,size=2):u.mem_write(address,value.to_bytes(size,'big'))
    def get(address,size=2):return int.from_bytes(u.mem_read(address,size),'big')
    def call(name,*args):
        stack=0xffdf00;stop=0x300000;u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join(x.to_bytes(4,'big') for x in args))
        try:u.emu_start(sy[name],stop,count=2000000)
        except Exception as e:raise AssertionError(name+' at '+hex(u.reg_read(UC_M68K_REG_PC))) from e
        assert u.reg_read(UC_M68K_REG_PC)==stop and u.reg_read(UC_M68K_REG_A7)==stack+4,name+' did not return cleanly'
        return u.reg_read(UC_M68K_REG_D0)
    for phase in [0,1,17,299]:
        for level in range(4):
            count=64<<level;repeat=1 if level<2 else 1<<(level-1)
            for method in range(2):assert call('pf_batch',phase,count,repeat,method)==oracle(phase,count,repeat)
    assert not call('pf_batch',300,64,1,0)
    w=sy['perf_window'];out=0xffe100
    put(out,999);assert call('pf_elapsed',4090,7,1,1,out)==1 and get(out)==13
    assert not call('pf_elapsed',4090,7,4,1,out) and get(out)==13
    assert call('pf_begin',w,65500,hz)==1
    for n in range(1,hz*4+1):assert call('pf_observe',w,(65500+n)&65535,1,1,10,1,1)==(2 if n==hz*4 else 1)
    assert get(w+4)==0 and get(w+6)==hz*4 and get(w+12)==hz*4
    reads=[];race=False
    def read(u,access,address,size,value,user):
        if address==0xa1200c:
            n=len(reads);reads.append(address);put(address,[4090,7,2,3][n%4])
            if race:put(sy['main_ticks'],get(sy['main_ticks'])+1)
    def write(u,access,address,size,value,user):
        assert not address<=0xa1200c<address+size,'Observer reset the shared stopwatch'
    u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    for name,value in [('hz',hz),('perf_case',0),('level',0),('method',1),('timer_on',1),('main_ticks',1)]:put(sy[name],value)
    call('pf_begin',w,0,hz);call('perf_sample',1)
    assert get(w+14)==1 and get(w+18)==13 and len(reads)==4
    assert get(sy['work_checksum'],4)==oracle(0,64,1)
    race=True;call('perf_sample',1);assert get(w+16)==1,'Incoherent stamp was accepted'
    race=False;put(sy['timer_on'],0);put(sy['main_ticks'],get(sy['main_ticks'])+1);before=len(reads)
    call('perf_sample',1);assert len(reads)==before,'Timer-off run still sampled stopwatch'
    # Pending ASIC/snapshot work must drain before private image storage can be reused.
    put(sy['run_request'],1);put(sy['desired_case'],1);put(sy['scale_phase'],2)
    snapshot=bytes(u.mem_read(sy['scale_pixels'],8192));call('perf_prepare_run');assert bytes(u.mem_read(sy['scale_pixels'],8192))==snapshot
    put(sy['scale_phase'],0);put(sy['scale_upload'],1);call('perf_prepare_run');assert bytes(u.mem_read(sy['scale_pixels'],8192))==snapshot
    put(sy['scale_upload'],0);call('perf_prepare_run');assert get(sy['preparing'])==1
    assert not get(w+4),'Measurement began before HUD warm-up was consumed'
    put(sy['dirty_count'],0);put(sy['published'],0);call('perf_prepare_run');assert get(w+4)==1 and not get(sy['run_request'])
    put(sy['hud'],0);put(sy['case_dirty'],0);snapshot=bytes(u.mem_read(sy['desired'],2240));count=get(sy['dirty_count'])
    call('scaling_draw');assert bytes(u.mem_read(sy['desired'],2240))==snapshot and get(sy['dirty_count'])==count
    call('scaling_input',128);assert not get(w+4) and get(sy['run_request'])==1 and not get(sy['measure_uploads'])
    # ASIC cases issue an unmeasured warm-up job before opening the window.
    put(sy['run_request'],1);put(sy['preparing'],0);put(sy['desired_case'],2)
    put(sy['scale_phase'],0);put(sy['scale_upload'],0);put(0xa12002,1)
    call('perf_prepare_run');assert get(sy['preparing'])==1 and not get(w+4)
    call('perf_prepare_run');assert get(sy['preparing'])==2 and not get(w+4) and get(sy['scale_phase'])==1
    assert not get(sy['measure_uploads']),'Warm-up payload entered measured accounting'
    # The image owns rows 4..19; summary labels must stay beneath it.
    for x,y,label in [(1,20,'WINDOW BLANKS'),(1,21,'STATUS'),(1,25,'CHECK')]:
        assert bytes(get(sy['desired']+((y*40+x+i)*2))&255 for i in range(len(label)))==label.encode(), 'Summary overlaps image pane'
    return True

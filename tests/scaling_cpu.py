"""Compiled 68000 ASIC adapters with explicit modeled completion/Word RAM.

This is a register, byte-layout and ownership model, not an ASIC timing emulator.
The oracle handles the Lab's supported 16px/256px, repeat, overwrite profile.
"""
from startup import check,symbols
import struct

def render(ram):
    out=bytearray(8192)
    for y in range(128):
        x0,y0,dx,dy=struct.unpack_from('>4H',ram,0x9400+y*8)
        dx=dx if dx<32768 else dx-65536;dy=dy if dy<32768 else dy-65536
        for x in range(128):
            sx=((x0<<8)+x*dx)//2048%256;sy=((y0<<8)+x*dy)//2048%256
            index=struct.unpack_from('>H',ram,0x9000+((sy//16)*16+sx//16)*2)[0]
            assert 1<=index<=256 and not index&0xF800
            off=index*128+(sx%16//8)*64+(sy%16)*4+(sx%8)//2
            c=(ram[off]>>(0 if sx&1 else 4))&15
            # ASIC image is columns of 8px-wide strips, each 128 pixels tall.
            pos=(x//8)*512+y*4+(x%8)//2
            out[pos]|=c<<(0 if x&1 else 4)
    return out

def check_scaling(project,hz):
    from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2
    contexts={}
    for file,kind in [('ipx.mmd','main'),('spx.smd','sub')]:
        table=symbols((project/'build'/(file+'.elf.sym')).read_text())
        def collect(u,sym,kind=kind):contexts[kind]=(u,sym);return True
        check((project/'disc'/file).read_bytes(),table,kind,collect)
    main,ms=contexts['main'];sub,ss=contexts['sub']
    sub.mem_map(0x80000,0x40000)
    for u,s in contexts.values():u.mem_map(0x300000,0x1000)
    main.mem_map(0xC00000,0x1000)
    ram=bytearray(262144);regs=bytearray(0x80);owner=1;grants=True;busy=False;started=0
    asic_writes=[];vdp=[];dma=[];blank=True;line=224;inject_token=0;inject_reply=0
    regs[0x33]=0x7E
    def word(u,p):return int.from_bytes(u.mem_read(p,2),'big')
    def put(u,p,v):u.mem_write(p,(v&65535).to_bytes(2,'big'))
    for kind,(u,s) in contexts.items():
        base=0xA12000 if kind=='main' else 0xFF8000;wr=0x200000 if kind=='main' else 0x80000
        def read(cpu,access,address,size,value,user,base=base,wr=wr,kind=kind):
            nonlocal owner,inject_token,inject_reply
            if kind=='sub' and address==base+0x10 and inject_token:
                # Main's grant and final token become visible after an old
                # speculative mode sample, exactly at the Sub token read.
                owner=2;regs[0x10:0x12]=inject_token.to_bytes(2,'big');inject_token=0
            if kind=='main' and address==base+0x20 and inject_reply:
                owner=1;regs[0x20:0x22]=inject_reply.to_bytes(2,'big');inject_reply=0
            if base<=address<base+0x80:
                regs[2:4]=owner.to_bytes(2,'big');regs[0x58:0x5a]=(1|(0x8000 if busy else 0)).to_bytes(2,'big')
                cpu.mem_write(address,bytes(regs[address-base:address-base+size]))
            if wr<=address<wr+262144:
                assert owner==(1 if kind=='main' else 2),'Read before grant/completion'
                cpu.mem_write(address,bytes(ram[address-wr:address-wr+size]))
            if address==0xC00004:cpu.mem_write(address,(8 if blank else 0).to_bytes(2,'big'))
            if address==0xC00008:cpu.mem_write(address,(line<<8).to_bytes(2,'big'))
        def write(cpu,access,address,size,value,user,base=base,wr=wr,kind=kind):
            nonlocal owner,busy,started
            value&=(1<<(size*8))-1
            if base<=address<base+0x80:
                off=address-base
                if off==3:
                    assert not busy,'Word RAM handed away while ASIC busy'
                    if grants:
                        if kind=='main' and value&2:owner=2
                        if kind=='sub' and value&1:owner=1
                elif off>=0x58:
                    assert kind=='sub' and owner==2 and not busy,'Unsafe ASIC configuration'
                    assert regs[0x33]==0x7C,'Polling owner failed to mask only ASIC INT1'
                    regs[off:off+size]=value.to_bytes(size,'big');asic_writes.append((off,value))
                    if off==0x66:
                        assert [o for o,v in asic_writes[-8:]]==[0x58,0x5a,0x5c,0x5e,0x60,0x62,0x64,0x66]
                        expected=[1,0x9000//4,15,0x10000//4,0,128,128,0x9400//4]
                        assert [v for o,v in asic_writes[-8:]]==expected,'Register units/layout mismatch'
                        busy=True;started+=1
                elif off==0x33:
                    assert kind=='sub' and value==0x7C
                    regs[off]=value
                elif 0x10<=off<0x30:
                    assert (kind=='main' and off<0x20) or (kind=='sub' and off>=0x20),'Wrong mailbox writer'
                    regs[off:off+size]=value.to_bytes(size,'big')
            if wr<=address<wr+262144:
                assert kind=='sub' and owner==2 and not busy,'CPU wrote leased or active ASIC RAM'
                ram[address-wr:address-wr+size]=value.to_bytes(size,'big')
            if 0xC00000<=address<0xC00010:vdp.append((address,size,value))
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
    def call(kind,name,*args):
        u,sym=contexts[kind];stack=0xFFDF00 if kind=='main' else 0x1FF00;stop=0x300000
        u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
        u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
        try:u.emu_start(sym[name],stop,count=2000000)
        except Exception as error:raise AssertionError(f'{kind} {name} PC {u.reg_read(UC_M68K_REG_PC):x}') from error
        assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' damaged stack'
        return u.reg_read(UC_M68K_REG_D0)
    def dma_call(u,address,size,user):
        source=u.reg_read(UC_M68K_REG_D1);words=u.reg_read(UC_M68K_REG_D2)&65535
        assert blank and 224<=line<=230 and words==2048
        assert ms['scale_pixels']<=source and source+words*2<=ms['scale_pixels']+8192
        dma.append((u.reg_read(UC_M68K_REG_D0),bytes(u.mem_read(source,words*2))))
        sp=u.reg_read(UC_M68K_REG_A7);u.reg_write(UC_M68K_REG_PC,int.from_bytes(u.mem_read(sp,4),'big'));u.reg_write(UC_M68K_REG_A7,sp+4)
    main.hook_add(UC_HOOK_CODE,dma_call,begin=0x2D0,end=0x2D0)
    request=0xFFE000;dest=0xFFE100
    # Execute target 16x16 multiply / trace builder and compare both CPU copies.
    for mode,angle,zoom in [(0,0,2),(1,0,2),(1,64,2),(1,127,0),(1,255,4),(2,0,2),(2,64,4),(2,191,0)]:
        blob=struct.pack('>4H',mode,angle,zoom,65535)
        main.mem_write(request,blob);sub.mem_write(0x1E000,blob)
        assert call('main','scale_trace',dest,128,request)==1
        assert call('sub','scale_trace',0x1E100,128,0x1E000)==1
        assert bytes(main.mem_read(dest,1024))==bytes(sub.mem_read(0x1E100,1024))
        if mode==0:assert bytes(main.mem_read(dest,8))==struct.pack('>4H',512,512,2048,0)
    main.mem_write(request,struct.pack('>4H',3,0,2,0));assert not call('main','scale_start',request)
    owner=5;main.mem_write(request,struct.pack('>4H',1,0,2,0))
    assert not call('main','scale_start',request),'Accepted 1M-mode ASIC request'
    owner=1
    for mode in range(3):
        main.mem_write(request,struct.pack('>4H',mode,16,2,37))
        assert call('main','scale_start',request)==1 and owner==2
        assert not call('main','scale_start',request),'Accepted overlapping render'
        call('main','scale_main_tick',300//hz)
        if mode==0:
            inject_token=int.from_bytes(regs[0x10:0x12],'big')
            regs[0x10:0x12]=bytes(2);owner=1
        call('sub','scale_sub_tick');assert busy
        before=bytes(ram);call('sub','scale_sub_tick')
        saved=bytes(regs[0x20:0x22]);regs[0x20:0x22]=(1).to_bytes(2,'big')
        call('main','scale_main_tick',300//hz);regs[0x20:0x22]=saved
        assert bytes(ram)==before and not word(main,ms['scale_upload']) and owner==2
        expected=render(ram)
        if mode==0:
            # Independent identity/crop check, including tile column order.
            art=__import__('test_project').load(project/'tools/make_art.py','target_art').art()
            for y in range(128):
                for x in range(128):
                    pos=(x//8)*512+y*4+(x%8)//2
                    pixel=(expected[pos]>>(0 if x&1 else 4))&15
                    assert pixel==art[y+64][x+64]
        ram[0x10000:0x12000]=expected;busy=False
        call('sub','scale_sub_tick');assert owner==1
        if mode==0:
            inject_reply=int.from_bytes(regs[0x20:0x22],'big');regs[0x20:0x22]=bytes(2);owner=2
        call('main','scale_main_tick',300//hz)
        assert bytes(main.mem_read(ms['scale_pixels'],8192))==expected
        assert word(main,ms['scale_upload'])==1
        call('sub','scale_sub_tick');call('main','scale_main_tick',300//hz)
        assert not word(main,ms['scale_phase'])
        snapshot=bytes(main.mem_read(ms['scale_pixels'],8192))
        call('main','scale_main_tick',300//hz)
        assert snapshot==bytes(main.mem_read(ms['scale_pixels'],8192))
        # Actual VBlank callback defers outside/late blank and flips only after
        # the complete two-chunk image has reached the hidden VRAM region.
        before_dma=len(dma);before_vdp=len(vdp);put(main,ms['published'],1)
        blank=False;call('main','vblank_user');assert len(dma)==before_dma
        blank=True;line=240;put(main,ms['published'],1);call('main','vblank_user');assert len(dma)==before_dma
        line=224;put(main,ms['published'],1);call('main','vblank_user')
        assert len(dma)==before_dma+1 and word(main,ms['scale_upload'])==1
        put(main,ms['published'],1);call('main','vblank_user')
        assert len(dma)==before_dma+2 and not word(main,ms['scale_upload'])
        assert b''.join(data for _,data in dma[-2:])==expected
        # 16 rows of 16 map words, distinct from the small sprite list.
        tail=vdp[before_vdp:];assert sum(1 for addr,size,val in tail if addr==0xC00000)==272
        assert word(main,ms['image_count'])==mode+1
    assert started==3 and word(main,ms['scale_error'])==0 and word(main,ms['views_seen'])==7
    # Pipeline a second ASIC job while the previous Main snapshot is leased
    # by DMA. Completion may return Word RAM, but the snapshot must stay stable.
    main.mem_write(request,struct.pack('>4H',1,3,2,0))
    def finish_job():
        nonlocal busy
        call('main','scale_main_tick',300//hz);call('sub','scale_sub_tick');assert busy
        ram[0x10000:0x12000]=render(ram);busy=False
        call('sub','scale_sub_tick');call('main','scale_main_tick',300//hz)
    assert call('main','scale_start',request);finish_job()
    call('sub','scale_sub_tick');call('main','scale_main_tick',300//hz)
    old=bytes(main.mem_read(ms['scale_pixels'],8192))
    main.mem_write(request,struct.pack('>4H',2,0,3,99))
    assert call('main','scale_start',request),'Did not overlap rendering with upload'
    finish_job()
    assert word(main,ms['scale_phase'])==2 and bytes(main.mem_read(ms['scale_pixels'],8192))==old
    new=bytes(ram[0x10000:0x12000]);assert new!=old
    for _ in range(2):put(main,ms['published'],1);call('main','vblank_user')
    call('main','scale_main_tick',300//hz)
    assert bytes(main.mem_read(ms['scale_pixels'],8192))==new
    call('sub','scale_sub_tick');call('main','scale_main_tick',300//hz)
    for _ in range(2):put(main,ms['published'],1);call('main','vblank_user')
    assert not word(main,ms['scale_upload']) and not word(main,ms['scale_phase'])
    # HUD changes coalesce, a no-change draw queues nothing, and shorter
    # labels erase their old trailing glyph rather than leaving stale text.
    put(main,ms['published'],0)
    call('main','scaling_draw');n=word(main,ms['dirty_count']);assert n>0
    call('main','scaling_draw');assert word(main,ms['dirty_count'])==n
    while word(main,ms['dirty_count']):
        call('main','scaling_publish');call('main','vblank_user')
    call('main','scaling_draw');call('main','scaling_publish')
    call('main','scaling_draw');assert not word(main,ms['dirty_count'])
    put(main,ms['request'],0);call('main','scaling_draw')
    assert word(main,ms['desired']+(5*40+5)*2)&255==32,'Short label left old trailing glyph'
    assert not word(main,ms['video_fault'])
    # A grant that never confirms times out without touching RAM or freeing it.
    grants=False;main.mem_write(request,struct.pack('>4H',1,0,2,0));before=bytes(ram)
    assert call('main','scale_start',request)
    for _ in range(hz*5+1):call('main','scale_main_tick',300//hz)
    assert word(main,ms['scale_phase'])==4 and bytes(ram)==before
    assert not call('main','scale_start',request)
    # Reset the fixture (not a runtime recovery API), then stall the ASIC.
    # A timeout cannot cancel hardware or hand away its active Word RAM.
    put(main,ms['scale_phase'],0);grants=True
    assert call('main','scale_start',request)
    call('main','scale_main_tick',300//hz);call('sub','scale_sub_tick');assert busy
    before=bytes(ram)
    for _ in range(hz*5+1):call('main','scale_main_tick',300//hz)
    call('sub','scale_sub_tick')
    assert busy and owner==2 and word(main,ms['scale_phase'])==4 and bytes(ram)==before
    assert not call('main','scale_start',request)
    return True

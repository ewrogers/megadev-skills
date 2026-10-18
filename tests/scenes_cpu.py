"""Execute the compiled scene policy and coordinator on the original 68000.

Completion events are modeled; this is not a full BIOS/VDP timing simulation.
"""
from startup import check,symbols

def check_scenes(project,hz):
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_SR,UC_M68K_REG_D0
    table=symbols((project/'build/ipx.mmd.elf.sym').read_text())
    def run(u,sym):
        u.mem_map(0x300000,0x1000)
        def call(name,*args):
            stack=0xFFDF00;stop=0x300000
            u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
            u.mem_write(stack,stop.to_bytes(4,'big')+b''.join((v&0xffffffff).to_bytes(4,'big') for v in args))
            u.emu_start(sym[name],stop,count=500000)
            assert u.reg_read(UC_M68K_REG_PC)==stop,name+' did not return'
            assert u.reg_read(UC_M68K_REG_A7)==stack+4,name+' damaged stack'
            return u.reg_read(UC_M68K_REG_D0)
        def put(address,value):u.mem_write(address,value.to_bytes(2,'big'))
        def word(address):return int.from_bytes(u.mem_read(address,2),'big')
        p=sym['flow'];field=lambda i:word(p+i*2)
        def reset():call('scene_init',p)
        def tick(elapsed,events=0,cached=0):call('scene_tick',p,elapsed,events,cached)
        reset();assert call('scene_request',p,1)==1
        for _ in range(hz*2//5):tick(300//hz)
        assert field(3)==1 and field(4)==0,'Fade depends on redraw rate or bypassed black acknowledgement'
        tick(0,1);assert field(3)==2
        tick(0,2,1);assert field(3)==4
        tick(0,16,1);tick(120);assert field(0)==1 and field(3)==0
        old=field(8);put(p+24,19)
        assert call('scene_hold',p,old)==1
        assert call('scene_request',p,2)==1;tick(120,1);tick(9000,2)
        assert field(3)==2 and field(7)==1 and field(12)==19
        assert call('scene_release',p,old-1)==0
        assert call('scene_release',p,old)==1
        tick(0,2);assert field(3)==3
        tick(0,8);tick(0,16);tick(120)
        assert field(0)==1 and field(8)==old and field(12)==19 and field(11)==1,'Failed load destroyed current scene'
        assert call('scene_request',p,2)==1
        assert call('scene_request',p,3)==2
        assert call('scene_request',p,4)==2
        tick(120,1);tick(0,2,1);tick(0,16);tick(120)
        assert field(0)==2 and field(1)==4 and field(3)==1
        assert call('scene_valid',p,old)==0,'Stale scene handle survived cleanup'
        # Exercise actual main-loop coordinator around an outstanding old read.
        reset();put(p,1);put(p+2,1);put(p+24,7)
        old=field(8);assert call('scene_hold',p,old)==1
        put(sym['preload_generation'],old);put(sym['active'],0)
        put(sym['cached']+2,1);put(sym['stream_phase'],2)
        put(0xA12002,1);put(0xA12010,0);put(0xA12020,0)
        assert call('scene_request',p,2)==1
        call('scene_service',120);call('scene_service',0)
        assert field(3)==2 and field(9)==1 and field(12)==7,'Live read released its scene lease'
        put(sym['stream_phase'],0);put(0xA12020,77)
        call('scene_service',0);assert field(3)==2 and field(9)==1,'Unretired mailbox authorized cleanup'
        put(0xA12020,0);call('scene_service',0)
        assert field(3)==4 and field(9)==0
        call('scene_service',0);assert word(sym['transition'])==2 and field(0)==1
        # Model the last map payload being consumed, not just queued.
        put(sym['transition'],3);put(sym['active'],1);put(sym['published'],1)
        call('scene_service',0);assert field(0)==1 and field(12)==7,'Queued map was mistaken for consumed map'
        put(sym['published'],0);call('scene_service',0)
        assert field(0)==2 and not field(12) and not word(sym['transition'])
        assert not call('scene_release',p,old),'Late callback released a new scene'
        # The actual frame builder must link a real 2x2 marker, not an opaque
        # HUD glyph. Its image is checked separately at native resolution.
        put(p,1);put(p+6,0);put(sym['booted'],1);put(sym['transition'],0);put(sym['stream_phase'],0)
        put(sym['published'],0);call('stream_prepare')
        sat=[word(sym['frames']+608+i*2) for i in range(8)]
        assert sat[1]==0x0601 and sat[4]>128 and sat[5]==0x0500 and sat[6]==0x808C and sat[7]==392,'Beacon sprite descriptor/chain is wrong'
        return True
    check((project/'disc/ipx.mmd').read_bytes(),table,'main',run)
    return True

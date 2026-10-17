"""Exercise the actual compiled asset decoder with valid/corrupt target bytes."""
def check_assets(u,sym,res):
    from unicorn.m68k_const import UC_M68K_REG_A7,UC_M68K_REG_PC,UC_M68K_REG_D0
    stop=0x300000;u.mem_map(stop,0x1000)
    stack=0xFFDF00;view=0xFFC000;source=0x200000
    def call(data):
        u.mem_write(source,data);u.mem_write(view-4,b'\xA5'*72)
        u.mem_write(stack,b''.join(x.to_bytes(4,'big') for x in [stop,source,len(data),view]))
        u.reg_write(UC_M68K_REG_A7,stack);u.emu_start(sym['asset_open'],stop,count=20000000)
        assert u.reg_read(UC_M68K_REG_PC)==stop,'asset decoder did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4,'asset decoder corrupted C stack'
        assert bytes(u.mem_read(view-4,4))==b'\xA5'*4
        assert bytes(u.mem_read(view+64,4))==b'\xA5'*4
        return u.reg_read(UC_M68K_REG_D0)&0xFFFF
    for name in ['garden','cavern','actor','panel']:
        data=(res/(name+'.mda')).read_bytes();assert call(data)==1,name
        assert bytes(u.mem_read(view,10))==data[6:16],'decoded geometry differs'
        bad=bytearray(data);bad[-1]^=1;assert call(bytes(bad))==0,'corrupt payload accepted'
        assert call(data[:40])==0,'truncated header accepted'
        bad=bytearray(data);bad[20:24]=(len(data)+2).to_bytes(4,'big');assert call(bytes(bad))==0,'out of range size accepted'
    return True

"""Run the compiled probe primitives on the original 68000 CPU model."""
def check_probe(u, sym, kind):
    from unicorn.m68k_const import UC_M68K_REG_A7, UC_M68K_REG_PC, UC_M68K_REG_D0
    stop=0x300000
    u.mem_map(stop,0x1000)
    scratch=0xFFC000 if kind=='main' else 0x1D100
    stack=0xFFDF00 if kind=='main' else 0x1FF00
    words=128
    u.mem_write(scratch-4,b'\xA5'*(words*2+8))
    def call(symbol,args):
        data=b''.join(int(value).to_bytes(4,'big') for value in [stop,*args])
        u.mem_write(stack,data);u.reg_write(UC_M68K_REG_A7,stack)
        u.emu_start(sym[symbol],stop,count=100000)
        assert u.reg_read(UC_M68K_REG_PC)==stop, symbol+' did not return'
        assert u.reg_read(UC_M68K_REG_A7)==stack+4, 'C callee corrupted its stack'
        return u.reg_read(UC_M68K_REG_D0)&0xFFFF
    # Known constants test the byte/word layout, not another copy of the helper.
    assert call('hw_pattern',[1,0x1234])==0x1335
    call('hw_fill',[scratch,words,0x1234])
    assert bytes(u.mem_read(scratch,4))==b'\x12\x34\x13\x35'
    assert call('hw_verify',[scratch,words,0x1234])==1
    u.mem_write(scratch+70,b'\xDE\xAD')
    assert call('hw_verify',[scratch,words,0x1234])==0
    assert call('hw_exercise',[scratch,words])==1
    assert call('hw_verify',[scratch,words,0x5555])==1
    assert call('hw_exercise',[scratch,0])==0
    assert bytes(u.mem_read(scratch-4,4))==b'\xA5'*4
    assert bytes(u.mem_read(scratch+words*2,4))==b'\xA5'*4
    return True

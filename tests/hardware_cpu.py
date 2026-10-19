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
    if kind=='main':
        check_usb(u,sym,call,scratch+512)
    return True


def check_usb(u,sym,call,base):
    """Execute the optional guest parser, never model MCU/USB availability."""
    import binascii
    import struct
    state,packet,reply=base,base+64,base+96
    # Selected m68k ABI: 4 longs, 2 words, 12 bytes, then four bool bytes.
    def flags():return bytes(u.mem_read(state+32,4))
    def wire(kind,nonce):
        body=b'HLV1'+struct.pack('>HI',kind,nonce)
        return body+struct.pack('>H',binascii.crc_hqx(body,0xffff))
    def feed(data,now):
        responses=[]
        for byte in data:
            if call('hl_ed_receive',[state,byte,now,reply])&0xff:
                responses.append(bytes(u.mem_read(reply,12)))
        return responses
    assert call('hl_ed_signature',[0x55a0,0x55a8])&0xff==1
    assert call('hl_ed_signature',[0xffff,0xffff])&0xff==0
    assert call('hl_ed_signature',[0x55a0,0x55a0])&0xff==0
    call('hl_ed_init',[state,0,60]);assert not feed(wire(1,123),10)
    call('hl_ed_init',[state,1,60]);assert flags()==b'\1\0\0\0'
    bad=bytearray(wire(1,123));bad[-1]^=1
    assert not feed(bad,10)
    assert feed(b'noise'+wire(1,0x12345678),11)==[wire(2,0x12345678^0xa55a5aa5)]
    assert not feed(wire(3,0x12345679),12)
    assert feed(wire(3,0x12345678),13)==[wire(4,0x12345678)]
    assert flags()==b'\1\0\1\1'
    call('hl_ed_tick',[state,312]);assert flags()[2]==1
    call('hl_ed_tick',[state,313]);assert flags()[2]==0
    assert feed(wire(1,3),320)==[wire(2,3^0xa55a5aa5)]
    assert not feed(wire(3,3),380)  # Expired pending challenge.
    call('hl_ed_packet',[packet,1,0x12345678])
    assert bytes(u.mem_read(packet,12))==wire(1,0x12345678)

"""Execute actual startup on M68000; no BIOS, Gate Array scheduling or drive model."""
import ctypes

def symbols(text):
    return {v[2]:int(v[0],16) for line in text.splitlines() if len(v:=line.split())==3}

def check(data,sym,kind):
    from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_PROT_ALL
    from unicorn.m68k_const import UC_CPU_M68K_M68000,UC_M68K_REG_SR,UC_M68K_REG_A7,UC_M68K_REG_PC
    u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
    keep=ctypes.create_string_buffer(65536)
    # Original 68000 addresses are 24-bit. Absolute-short accesses sign-extend;
    # map the same backing RAM at both spellings, not two independent copies.
    for base in [0xFF0000,0xFFFF0000]:u.mem_map_ptr(base,65536,UC_PROT_ALL,ctypes.addressof(keep))
    u.mem_map(0,0x80000);u.mem_map(0x200000,0x40000);u.mem_map(0xA12000,0x1000)
    u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,0xFFE000)
    bss,length=sym['_BSS_ORIGIN'],sym['_BSS_LENGTH']
    ram,size=sym['_RAM_DATA_ORIGIN'],sym['_RAM_DATA_LENGTH']
    u.mem_write(bss-4,b'\xA5'*(length+size+8))
    expected=data[-size:] if size else b''
    assert u.mem_read(bss,length)==b'\xA5'*length,'poison not installed'
    assert u.mem_read(ram,size)!=expected,'negative startup control not distinguishable'
    if kind=='sub':
        u.mem_write(0x10000,data);entry=0x10000;stack=0x20000
    else:
        u.mem_write(0x200000,data)
        u.mem_write(0xA12002,b'\x00\x01') # Main owns 2M RAM for INIT_MMD's access check.
        entry=0x200100 if kind=='main' else int.from_bytes(data[8:12],'big')
        stack=0xFFE000
    u.emu_start(entry,sym['main'],count=200000)
    assert u.reg_read(UC_M68K_REG_PC)==sym['main'],'startup did not reach C main'
    assert u.reg_read(UC_M68K_REG_A7)==stack,'unexpected C-entry stack'
    assert bytes(u.mem_read(bss,length))==bytes(length),'BSS not cleared'
    assert bytes(u.mem_read(ram,size))==expected,'initialized data not copied'
    assert bytes(u.mem_read(bss-4,4))==b'\xA5'*4,'startup wrote below BSS'
    assert bytes(u.mem_read(ram+size,4))==b'\xA5'*4,'startup wrote past data'
    return {'bss_bytes':length,'data_bytes':size,'stack_top':stack,'main':sym['main']}

def check_sub_callback(data,sym):
    from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN
    from unicorn.m68k_const import UC_CPU_M68K_M68000,UC_M68K_REG_SR,UC_M68K_REG_A7,UC_M68K_REG_PC
    u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000)
    u.mem_map(0,0x80000);u.mem_map(0x300000,0x1000);u.mem_map(0xFF8000,0x8000)
    u.mem_write(0x6000,data);u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,0x1FFF0)
    u.mem_write(0x1FFF0,(0x300000).to_bytes(4,'big'))
    # PROCESS_ACC_LOOP tail-jumps to its continuation. A harmless RTS models
    # that continuation here; the actual CD access state machine is not modeled.
    u.mem_write(0x300100,b'\x4E\x75');u.mem_write(sym['acc_loop_jump'],(0x300100).to_bytes(4,'big'))
    address=0xFF8026; # GA_REG_COMSTAT3 in the pinned Sub Gate Array header.
    u.mem_write(address,(41).to_bytes(2,'big'))
    u.emu_start(sym['sp_int2'],0x300000,count=100)
    assert u.reg_read(UC_M68K_REG_PC)==0x300000,'Sub callback did not return'
    assert int.from_bytes(u.mem_read(address,2),'big')==42,'Sub heartbeat skipped before CD tail-jump'
    return True

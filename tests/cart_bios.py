#!/usr/bin/env python3
"""Optional local firmware check on fresh in-memory cart RAM, never user saves.

BIOS is user-supplied and never distributed. CI uses save_cpu's BIOS-ABI model.
This fixture supports the audited US ROM hash only; it does not boot a console
or qualify electrical timing. Format runs only against a new Python bytearray.
"""
import argparse
import hashlib
from pathlib import Path
from startup import check,symbols
HASH='b1c2036b79467514eaaa69c76e5ba83801621e821059caa53cb921a0b21e3af4'
def validate(project,bios_path,ident=4):
    from unicorn import UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
    from unicorn.m68k_const import UC_M68K_REG_PC,UC_M68K_REG_A7,UC_M68K_REG_SR,UC_M68K_REG_D0
    rom=Path(bios_path).read_bytes()
    if ident not in range(1,7):raise ValueError('Standard size ID required')
    if hashlib.sha256(rom).hexdigest()!=HASH:raise ValueError('This optional fixture requires the locally audited US BIOS; no guessed entry offsets')
    sym=symbols((project/'build/ipx.mmd.elf.sym').read_text())
    def run(u,s):
        u.mem_write(0,rom);u.mem_map(0x300000,0x1000);u.mem_map(0x400000,0x400000)
        # The boot-installed Main vector for this exact firmware revision.
        u.mem_write(0xFFFDAE,bytes.fromhex('4ef9000088f0'))
        ram=bytearray(b'\xff'*(8192<<ident));enabled=False
        def byte(address):
            if not address&1:return 255
            if address<0x500000:return ident
            if 0x600000<=address<0x700000:return ram[(address>>1)&(len(ram)-1)]
            if address>=0x700000:return int(enabled)
            return 255
        def read(cpu,access,address,size,value,user):
            if 0x400000<=address<0x800000:cpu.mem_write(address,bytes(byte(address+i) for i in range(size)))
        def write(cpu,access,address,size,value,user):
            nonlocal enabled
            if not 0x400000<=address<0x800000:return
            for i,b in enumerate((value&((1<<(size*8))-1)).to_bytes(size,'big')):
                p=address+i
                if not p&1:continue
                if 0x700000<=p<0x800000:enabled=bool(b&1)
                elif 0x600000<=p<0x700000 and enabled:ram[(p>>1)&(len(ram)-1)]=b
        u.hook_add(UC_HOOK_MEM_READ,read);u.hook_add(UC_HOOK_MEM_WRITE,write)
        def call(name,*args,d0=None):
            stack=0xFFDF00;stop=0x300000
            u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,stack)
            u.mem_write(stack,stop.to_bytes(4,'big')+b''.join(a.to_bytes(4,'big') for a in args))
            if d0 is not None:u.reg_write(UC_M68K_REG_D0,d0)
            try:u.emu_start(s[name] if isinstance(name,str) else name,stop,count=20000000)
            except Exception as error:raise AssertionError(f'{name}: PC {u.reg_read(UC_M68K_REG_PC):x}') from error
            assert u.reg_read(UC_M68K_REG_PC)==stop,'BIOS did not return'
            return u.reg_read(UC_M68K_REG_D0)
        assert call('cart_init')==1,'Fresh in-memory cart should be unformatted'
        game=0xFFDC00
        def request(op,values):
            u.mem_write(game,b''.join(x.to_bytes(2,'big') for x in values))
            u.mem_write(s['save_device'],b'\0\1')
            assert call('save_request',op,game)==1
            return int.from_bytes(u.mem_read(s['save_result'],2),'big')
        assert request(5,[1,0,0])==13,'Compiled format adapter failed on fresh cart'
        assert call('cart_init')==3
        assert request(2,[2,17,1])==1
        assert request(2,[4,31,3])==1
        assert request(3,[1,0,0])==2 and int.from_bytes(u.mem_read(s['save_reply'],2),'big')==4
        assert request(4,[1,0,0])==3 and int.from_bytes(u.mem_read(s['save_reply'],2),'big')==2
        assert request(5,[1,0,0])==13
        assert request(3,[1,0,0])==4,'Format left old Lab records behind'
        assert request(2,[3,18,2])==1 and request(3,[1,0,0])==2
        assert not enabled,'BIOS left cart writes enabled after returning'
        return True
    check((project/'disc/ipx.mmd').read_bytes(),sym,'main',run)
    return True
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('project',type=Path);p.add_argument('--bios',type=Path,required=True);p.add_argument('--id',type=int,default=4,choices=range(1,7));a=p.parse_args()
    validate(a.project,a.bios,a.id);print(f'Local BIOS + original-68000 Main adapters: cart ID {a.id} saves, readback and recovery passed on fresh in-memory RAM')

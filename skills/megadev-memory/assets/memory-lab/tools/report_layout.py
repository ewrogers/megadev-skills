#!/usr/bin/env python3
"""Check linked Megadev module usage against this project's explicit RAM reservations."""
import argparse
import json
from pathlib import Path
def number(value):
    if isinstance(value,bool) or not isinstance(value,(int,str)):raise ValueError('Expected an integer byte count/address')
    try:result=int(value,0) if isinstance(value,str) else value
    except ValueError:raise ValueError('Invalid integer') from None
    if result<0 or result>0xffffffff:raise ValueError('Address/count outside 32 bits')
    return result
def symbols(path):
    result={}
    for line in path.read_text().splitlines():
        parts=line.split()
        if len(parts)!=3:continue
        try:value=int(parts[0],16)
        except ValueError:continue
        name=parts[2]
        if name in result and result[name]!=value:raise ValueError('Conflicting symbol '+name)
        result[name]=value
    return result
def report(root,config=None):
    root=Path(root).resolve();config=config if config is not None else json.loads((root/'layout.json').read_text())
    tables={};regions={};pools=[];seen=set()
    def table(module):
        if not isinstance(module,str) or '/' in module or '\\' in module or module in ('.','..'):raise ValueError('Invalid module filename')
        if module not in tables:tables[module]=symbols(root/'build'/(module+'.elf.sym'))
        return tables[module]
    for pool in config['pools']:
        origin=number(pool['start']);capacity=number(pool['bytes']);end=origin+capacity
        kind=pool['kind']
        if kind in seen or {'main':(0xFF0000,65536),'sub':(0,524288)}.get(kind)!=(origin,capacity):raise ValueError('Incorrect or duplicate physical CPU pool')
        seen.add(kind)
        if not capacity or end>0x100000000:raise ValueError('Invalid physical pool')
        rows=[];used_reservations=0
        for region in pool['regions']:
            name=region['name'];start=number(region['start']);size=number(region['bytes']);used=None
            if name in regions:raise ValueError('Duplicate region '+name)
            if not size or start<origin or start+size>end:raise ValueError('Reservation outside '+pool['name'])
            if any(start<row['start']+row['reserved'] and row['start']<start+size for row in rows):raise ValueError('Overlapping reservation '+name)
            if 'module' in region:
                s=table(region['module']);section=region['section']
                try:
                    if section in ('rom','ram'):
                        prefix='MODULE_ROM' if section=='rom' else 'MODULE_RAM'
                        if s[prefix+'_ORIGIN']!=start or s[prefix+'_LENGTH']>size:raise ValueError('Linker region differs from reservation '+name)
                        used=(s['_ROM_DATA_ORIGIN']+s['_RAM_DATA_LENGTH']-start if section=='rom' else s['_RAM_DATA_ORIGIN']+s['_RAM_DATA_LENGTH']-start)
                    elif section=='boot':
                        if s['_TEXT_ORIGIN']!=start:raise ValueError('Bootstrap origin differs')
                        used=max(s['_TEXT_ORIGIN']+s['_TEXT_LENGTH'],s['_RAM_ORIGIN']+s['_RAM_LENGTH'])-start
                    else:raise ValueError('Unknown section '+section)
                except KeyError as error:raise ValueError('Missing required linker symbol '+str(error)) from error
                if used<0 or used>size:raise ValueError('Linked bytes exceed '+name)
            row={'name':name,'start':start,'reserved':size,'linked_bytes':used}
            rows.append(row);regions[name]=row;used_reservations+=size
        pools.append({'name':pool['name'],'capacity':capacity,'reserved':used_reservations,'unassigned':capacity-used_reservations,'regions':rows})
    embedded=[]
    for item in config.get('embedded',[]):
        s=table(item['module']);region=regions[item['region']];size=number(item['bytes'])
        if item['symbol'] not in s:raise ValueError('Missing embedded buffer '+item['symbol'])
        start=s[item['symbol']]
        linked=region['linked_bytes']
        if linked is None or not size or start<region['start'] or start+size>region['start']+linked:raise ValueError('Embedded buffer outside linked state')
        embedded.append({'name':item['name'],'bytes':size,'already_counted':True})
    payload=number(config['word_bank_payload_bytes'])
    if not payload or payload>131072:raise ValueError('Payload exceeds one 1M bank')
    return {'pools':pools,'embedded':embedded,'word_ram':{'physical_bytes':262144,'bank_1m_bytes':131072,'payload_per_bank':payload},
            'limits':'Reservations are project policy; stacks are reserved, not measured. Unassigned RAM is not an initialized heap. Linked state already includes embedded pools.'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=Path('.'));a=p.parse_args()
    try:print(json.dumps(report(a.root),indent=2))
    except (ValueError,KeyError,OSError,TypeError) as error:p.exit(1,str(error)+'\n')

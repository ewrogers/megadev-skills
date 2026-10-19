#!/usr/bin/env python3
"""Compare recorded Lab counters; computes rates, never CPU utilization."""
import argparse
import json
from pathlib import Path

def validate(r):
    required={'schema','case','level','method','hud','timer','hz','elapsed','loops','late','blocks','upload_kib','images','check_ok'}
    if not isinstance(r,dict) or set(r)!=required:raise ValueError('Need exactly the documented run fields')
    for key in ('schema','case','level','method','hz','elapsed','loops','late','blocks','upload_kib','images'):
        if type(r[key]) is not int or not 0<=r[key]<=65535:raise ValueError('Invalid counter: '+key)
    for key in ('hud','timer','check_ok'):
        if type(r[key]) is not bool:raise ValueError('Expected boolean: '+key)
    if r['schema']!=1 or r['case']>3 or r['level']>3 or r['method']>1 or r['hz'] not in (50,60):raise ValueError('Unsupported run configuration')
    if not r['check_ok'] or not r['loops'] or not 4*r['hz']<=r['elapsed']<=4*r['hz']+32:raise ValueError('Incomplete or failed run')
    return r

def compare(a,b,vary):
    a=validate(a);b=validate(b)
    if vary not in ('method','hud','timer','region'):raise ValueError('Choose one comparison axis')
    axis='hz' if vary=='region' else vary
    for key in ('case','level','method','hud','timer','hz'):
        if key!=axis and a[key]!=b[key]:raise ValueError('More than one configuration changed: '+key)
    metrics={}
    for key in ('loops','blocks','upload_kib','images'):
        rates=[r[key]*r['hz']/r['elapsed'] for r in (a,b)]
        metrics[key]={'a_per_second':rates[0],'b_per_second':rates[1],'b_over_a':rates[1]/rates[0] if rates[0] else None}
    return {'axis':vary,'metrics':metrics,'limits':'Nominal display rate and serviced VBlanks; recorded counters are not CPU utilization, cycles or a hardware certification.'}
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('a',type=Path);p.add_argument('b',type=Path);p.add_argument('--vary',required=True,choices=['method','hud','timer','region']);a=p.parse_args()
    try:print(json.dumps(compare(json.loads(a.a.read_text()),json.loads(a.b.read_text()),a.vary),indent=2))
    except (ValueError,OSError) as e:p.exit(2,str(e)+'\n')

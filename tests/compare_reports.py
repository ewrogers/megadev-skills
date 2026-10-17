#!/usr/bin/env python3
"""Fail closed unless both architectures pass the same complete target matrix."""
import argparse
import json
from pathlib import Path
CASES={'us-release','jp-release','eu-release','us-debug'}
def compare(paths):
    reports=[json.loads(Path(p).read_text()) for p in paths]
    if len(reports)!=2 or {r.get('platform') for r in reports}!={'linux/arm64','linux/amd64'}:
        raise ValueError('Need one ARM64 and one AMD64 report')
    kinds={r.get('skill','megadev-project') for r in reports}
    if len(kinds)!=1 or not kinds <= {'megadev-project','megadev-hardware'}:
        raise ValueError('Need matching supported skill reports')
    expected=CASES | ({'us-tour'} if 'megadev-hardware' in kinds else set())
    for r in reports:
        if r.get('passed') is not True or set(r.get('cases',{}))!=expected:
            raise ValueError('Failed or incomplete matrix')
        for case in r['cases'].values():
            if case.get('sub_callback') is not True or set(case.get('startup',{}))!={'main','sub','room'}:
                raise ValueError('Missing startup/callback checks')
            if 'megadev-hardware' in kinds and any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')):
                raise ValueError('Missing executed hardware probes')
            if not case.get('boot_payloads') or not case.get('disc',{}).get('files'):
                raise ValueError('Missing target payload hashes')
    if reports[0]['cases']!=reports[1]['cases']:
        raise ValueError('Target payloads or startup results differ across architectures')
    return True
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reports',nargs=2,type=Path);a=p.parse_args()
    compare(a.reports);print('ARM64 / AMD64: complete target matrices match')

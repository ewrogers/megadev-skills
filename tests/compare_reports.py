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
    if len(kinds)!=1 or not kinds <= {'megadev-project','megadev-hardware','megadev-assets','megadev-graphics','megadev-input','megadev-system','megadev-memory','megadev-streaming','megadev-audio','megadev-scenes','megadev-save','megadev-scaling','megadev-debug','megadev-testing','megadev-performance','megadev-best-practices','megadev-ui'}:
        raise ValueError('Need matching supported skill reports')
    expected=CASES | ({'us-tour'} if kinds & {'megadev-hardware','megadev-assets','megadev-graphics','megadev-input','megadev-system','megadev-memory','megadev-streaming','megadev-audio','megadev-scenes','megadev-save','megadev-scaling','megadev-debug','megadev-testing','megadev-performance','megadev-best-practices','megadev-ui'} else set())
    if 'megadev-input' in kinds:expected|={'us-ea'}
    for r in reports:
        if r.get('passed') is not True or set(r.get('cases',{}))!=expected:
            raise ValueError('Failed or incomplete matrix')
        for case in r['cases'].values():
            if case.get('sub_callback') is not True or set(case.get('startup',{}))!={'main','sub','room'}:
                raise ValueError('Missing startup/callback checks')
            if 'megadev-hardware' in kinds and any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')):
                raise ValueError('Missing executed hardware probes')
            if 'megadev-assets' in kinds and case['startup']['main'].get('probe_execution') is not True:
                raise ValueError('Missing executed asset decoder checks')
            if 'megadev-assets' in kinds and not {'GARDEN.MDA','CAVERN.MDA'} <= case.get('disc',{}).get('files',{}).keys():
                raise ValueError('Missing disc art')
            if 'megadev-graphics' in kinds and (case['startup']['main'].get('probe_execution') is not True or not {'SKY.MDA','QUAY.MDA'} <= case.get('disc',{}).get('files',{}).keys()):
                raise ValueError('Missing executed graphics checks or disc layers')
            if 'megadev-input' in kinds and case['startup']['main'].get('probe_execution') is not True:
                raise ValueError('Missing executed input protocol/IRQ checks')
            if 'megadev-system' in kinds and any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')):
                raise ValueError('Missing paired executed system checks')
            if 'megadev-memory' in kinds:
                layout=case['startup']['main'].get('layout',{})
                if any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or len(layout.get('pools',[]))!=2 or layout.get('word_ram',{}).get('bank_1m_bytes')!=131072:
                    raise ValueError('Missing executed memory checks or linked layout')
            if 'megadev-streaming' in kinds:
                if any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or not {'GARDEN.MDA','CAVERN.MDA'} <= case.get('disc',{}).get('files',{}).keys():
                    raise ValueError('Missing executed streaming adapters or disc areas')
            if kinds & {'megadev-audio','megadev-scenes','megadev-testing','megadev-best-practices'}:
                if any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or set(case.get('audio_tracks',{}))!={'2','3'} or not {'GARDEN.MDA','CAVERN.MDA'} <= case.get('disc',{}).get('files',{}).keys():
                    raise ValueError('Missing executed audio adapters, CD tracks or disc areas')
            if kinds & {'megadev-audio','megadev-best-practices'}:
                release=case.get('release',{})
                if release.get('profile')!='megadev-multifile-v1' or release.get('round_trip') is not True or release.get('reproducible_package') is not True or set(release.get('negative_cases',[]))!={'missing_track','changed_audio','changed_pregap','wrong_track_path','security_damage','damaged_package'} or len(release.get('tracks',[]))!=3:
                    raise ValueError('Missing mixed-mode release/package qualification')
            if kinds & {'megadev-scenes','megadev-best-practices'} and case['startup']['main'].get('scene_execution') is not True:
                raise ValueError('Missing executed scene policy/coordinator checks')
            if 'megadev-save' in kinds and (any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or case['startup']['main'].get('cart_execution') is not True):
                raise ValueError('Missing executed save service and BRAM ABI checks')
            if 'megadev-scaling' in kinds and (any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or case['startup']['sub'].get('asic_execution') is not True):
                raise ValueError('Missing executed ASIC adapters, completion and publication checks')
            if 'megadev-debug' in kinds and (any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or case['startup']['main'].get('debug_execution') is not True):
                raise ValueError('Missing executed diagnostic and paired coordination checks')
            if 'megadev-testing' in kinds and any(case['startup']['main'].get(marker) is not True for marker in ('replay_execution','integration_execution')):
                raise ValueError('Missing executed replay and combined integration regressions')
            if 'megadev-performance' in kinds and (any(case['startup'][kind].get('probe_execution') is not True for kind in ('main','sub')) or case['startup']['main'].get('measurement_execution') is not True or case['startup']['sub'].get('asic_execution') is not True):
                raise ValueError('Missing executed benchmark observer or ASIC qualification')
            if 'megadev-best-practices' in kinds and (case['startup']['main'].get('practice_execution') is not True or case['startup']['sub'].get('checkpoint_execution') is not True):
                raise ValueError('Missing executed integration/BIOS checkpoint evidence')
            if 'megadev-ui' in kinds and (case['startup']['main'].get('ui_execution') is not True or case['startup']['main'].get('probe_execution') is not True):
                raise ValueError('Missing executed UI/publication qualification')
            if not case.get('boot_payloads') or not case.get('disc',{}).get('files'):
                raise ValueError('Missing target payload hashes')
    if reports[0]['cases']!=reports[1]['cases']:
        raise ValueError('Target payloads or startup results differ across architectures')
    return True
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reports',nargs=2,type=Path);a=p.parse_args()
    compare(a.reports);print('ARM64 / AMD64: complete target matrices match')

#!/usr/bin/env python3
"""Check declared lifetime budgets; never infer measured performance from sizes."""
import argparse
import json
import math
from pathlib import Path
import sys

CAPACITY = {'work': 65536, 'program': 524288, 'word': 262144,
            'pcm': 65536, 'vram': 65536, 'z80': 8192}
OWNERS = {'work': {'main'}, 'program': {'sub'}, 'word': {'main', 'sub'},
          'pcm': {'pcm'}, 'vram': {'vdp'}, 'z80': {'z80'}}
EVIDENCE = {'estimate', 'measured', 'reservation'}


class InvalidPlan(ValueError):
    pass


def fields(obj, required, optional, where):
    if not isinstance(obj, dict):
        raise InvalidPlan(where + ': expected an object')
    missing, unknown = set(required) - obj.keys(), obj.keys() - set(required) - set(optional)
    if missing or unknown:
        raise InvalidPlan(f'{where}: missing {sorted(missing)}, unknown {sorted(unknown)}')


def integer(value, where, minimum=0):
    if type(value) is not int or value < minimum:
        raise InvalidPlan(f'{where}: expected integer >= {minimum}')
    return value


def number(value, where, minimum=0, positive=False):
    try:
        valid = type(value) in (int, float) and math.isfinite(value) and value >= minimum and (not positive or value > 0)
    except OverflowError:
        valid = False
    if not valid:
        raise InvalidPlan(where + ': expected a finite nonnegative number' + (' greater than zero' if positive else ''))
    return value


def name(value, where):
    if not isinstance(value, str) or not value.strip():
        raise InvalidPlan(where + ': expected a nonempty name')
    return value


def array(value, where):
    if not isinstance(value, list):
        raise InvalidPlan(where + ': expected an array')
    return value


def check(plan):
    fields(plan, ['version', 'display', 'phases', 'allocations'], ['loads', 'notes'], 'plan')
    if type(plan['version']) is not int or plan['version'] != 1:
        raise InvalidPlan('Unsupported plan version')
    display = plan['display']
    fields(display, ['mode', 'refresh_hz'], [], 'display')
    if display['mode'] not in ('H32', 'H40') or type(display['refresh_hz']) is not int or display['refresh_hz'] not in (50, 60):
        raise InvalidPlan('Display needs H32/H40 and 50/60 Hz')
    phases = {}
    for p in array(plan['phases'], 'phases'):
        fields(p, ['name', 'word_mode'], ['word_owner', 'graphics', 'cpu_us', 'notes'], 'phase')
        key = name(p['name'], 'phase.name')
        if key in phases:
            raise InvalidPlan('Duplicate phase: ' + key)
        if p['word_mode'] not in ('2m', '1m'):
            raise InvalidPlan('Word mode must be 2m or 1m')
        if p['word_mode'] == '2m' and p.get('word_owner') not in ('main', 'sub'):
            raise InvalidPlan('2m mode needs a main/sub word_owner')
        if p['word_mode'] == '1m' and 'word_owner' in p:
            raise InvalidPlan('1m uses one bank per CPU; omit word_owner')
        phases[key] = p
    if not phases:
        raise InvalidPlan('Declare at least one phase')
    allocations, ids = [], set()
    for a in array(plan['allocations'], 'allocations'):
        fields(a, ['name', 'pool', 'bytes', 'owner', 'phases', 'evidence'], ['count', 'notes'], 'allocation')
        key = name(a['name'], 'allocation.name')
        if key in ids:
            raise InvalidPlan('Duplicate allocation: ' + key)
        ids.add(key)
        pool = a['pool']
        if not isinstance(pool, str) or pool not in CAPACITY or not isinstance(a['owner'], str) or a['owner'] not in OWNERS[pool]:
            raise InvalidPlan(key + ': invalid pool or owner')
        size = integer(a['bytes'], key + '.bytes') * integer(a.get('count', 1), key + '.count', 1)
        active = array(a['phases'], key + '.phases')
        if not active or any(not isinstance(x, str) for x in active) or len(set(active)) != len(active) or set(active) - phases.keys():
            raise InvalidPlan(key + ': phases must be unique known names')
        if not isinstance(a['evidence'], str) or a['evidence'] not in EVIDENCE:
            raise InvalidPlan(key + ': evidence must be estimate, measured or reservation')
        allocations.append((a, size))
    errors, warnings, result = [], [], {}
    for key, p in phases.items():
        used = dict.fromkeys(CAPACITY, 0)
        word = {'main': 0, 'sub': 0}
        estimates = []
        for a, size in allocations:
            if key not in a['phases']:
                continue
            used[a['pool']] += size
            if a['evidence'] == 'estimate':
                estimates.append(a['name'])
            if a['pool'] == 'word':
                word[a['owner']] += size
                if size and p['word_mode'] == '2m' and a['owner'] != p['word_owner']:
                    errors.append(f'{key}: {a["name"]} needs {a["owner"]} access while {p["word_owner"]} owns 2m Word RAM')
        for pool, size in used.items():
            if size > CAPACITY[pool]:
                errors.append(f'{key}: {pool} exceeds capacity by {size - CAPACITY[pool]} bytes')
        if p['word_mode'] == '1m':
            for owner, size in word.items():
                if size > 131072:
                    errors.append(f'{key}: {owner} Word RAM bank exceeds 128 KiB by {size - 131072} bytes')
        if estimates:
            warnings.append(key + ': unmeasured allocations: ' + ', '.join(estimates))
        graphics = p.get('graphics')
        if graphics is None:
            warnings.append(key + ': sprite and palette demand not assessed')
        else:
            keys = ['palette_lines', 'sprites', 'peak_line_sprites', 'peak_line_pixels']
            fields(graphics, keys, [], key + '.graphics')
            limits = [4, 80, 20, 320] if display['mode'] == 'H40' else [4, 64, 16, 256]
            for metric, limit in zip(keys, limits):
                value = integer(graphics[metric], key + '.' + metric)
                if value > limit:
                    errors.append(f'{key}: {metric} {value} exceeds {limit}')
        cpu = p.get('cpu_us')
        if cpu is not None:
            fields(cpu, ['main', 'sub'], [], key + '.cpu_us')
            for owner in ('main', 'sub'):
                value = number(cpu[owner], key + '.cpu_us.' + owner)
                if value > 1000000 / display['refresh_hz']:
                    errors.append(f'{key}: declared {owner} CPU time exceeds the frame period')
        else:
            warnings.append(key + ': CPU timing not measured')
        result[key] = {'used_bytes': used, 'free_bytes': {k: CAPACITY[k] - v for k, v in used.items()},
                       'word_bytes': word}
    loads = []
    load_ids = set()
    for load in array(plan.get('loads', []), 'loads'):
        keys = ['name', 'packed_bytes', 'assumed_seek_ms', 'assumed_bytes_per_second',
                'assumed_decode_ms', 'upload_frames', 'available_ms']
        fields(load, keys, ['notes'], 'load')
        key = name(load['name'], 'load.name')
        if key in load_ids:
            raise InvalidPlan('Duplicate load: ' + key)
        load_ids.add(key)
        packed = integer(load['packed_bytes'], key + '.packed_bytes')
        seek = number(load['assumed_seek_ms'], key + '.assumed_seek_ms')
        rate = number(load['assumed_bytes_per_second'], key + '.assumed_bytes_per_second', positive=True)
        decode = number(load['assumed_decode_ms'], key + '.assumed_decode_ms')
        frames = integer(load['upload_frames'], key + '.upload_frames')
        window = number(load['available_ms'], key + '.available_ms')
        try:
            total = seek + 1000 * packed / rate + decode + frames * 1000 / display['refresh_hz']
        except OverflowError as error:
            raise InvalidPlan(key + ': load arithmetic overflow') from error
        if not math.isfinite(total):
            raise InvalidPlan(key + ': load arithmetic must stay finite')
        loads.append({'name': key, 'modeled_ms': total, 'margin_ms': window - total})
        warnings.append(key + ': load time is an assumption-based serial model, not a drive measurement')
        if total > window:
            warnings.append(key + ': modeled load misses the preload window; plan a fallback')
    warnings.append('Totals cover declared allocations only; no address, alignment, DMA schedule or physical-hardware proof')
    return {'ok': not errors, 'errors': errors, 'warnings': warnings, 'phases': result,
            'peak_bytes': {pool: max(p['used_bytes'][pool] for p in result.values()) for pool in CAPACITY},
            'loads': loads}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('plan', type=Path)
    parser.add_argument('--json', action='store_true', help='Print a structured report')
    args = parser.parse_args()
    try:
        report = check(json.loads(args.plan.read_text(encoding='utf-8')))
    except (OSError, ValueError, TypeError) as error:
        print('Invalid plan: ' + str(error), file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print('Declared budgets: ' + ('PASS' if report['ok'] else 'FAIL'))
        for phase, values in report['phases'].items():
            print(phase + ': ' + ', '.join(f'{p} {n}/{CAPACITY[p]}' for p, n in values['used_bytes'].items()))
        for load in report['loads']:
            print(f'{load["name"]}: modeled {load["modeled_ms"]:.1f} ms, margin {load["margin_ms"]:.1f} ms')
        for label in ('errors', 'warnings'):
            for message in report[label]:
                print(label[:-1].upper() + ': ' + message)
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

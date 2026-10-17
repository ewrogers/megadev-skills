#!/usr/bin/env python3
"""Verify prepared-art hashes and binary structure using only the standard library."""
import argparse
import hashlib
import json
from pathlib import Path
from asset_codec import inspect


def verify(root, reports):
    root=Path(root).resolve()
    if not reports:raise ValueError('No asset reports supplied')
    for report in reports:
        path=Path(report).resolve();m=json.loads(path.read_text(encoding='utf-8'))
        if m.get('version')!=1 or not m.get('inputs') or not m.get('outputs'):
            raise ValueError('Invalid asset report')
        for item in m['inputs']+m['outputs']:
            target=(path.parent/item['path']).resolve()
            if not target.is_relative_to(root):raise ValueError('Asset path escapes project root')
            if hashlib.sha256(target.read_bytes()).hexdigest()!=item['sha256']:
                raise ValueError('Stale asset: '+str(target))
            if target.suffix=='.mda':
                info=inspect(target.read_bytes())
                if any(info[k]!=m['asset'][k] for k in ['kind','tiles','entries','tile_bytes','total_bytes','crc32']):
                    raise ValueError('Asset report does not match its binary')
    return True


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--header',type=Path);p.add_argument('reports',nargs='+',type=Path);a=p.parse_args()
    try:
        verify(a.root,a.reports)
        if a.header:
            lines=['/* Generated from verified prepared assets. */']
            for report in sorted(a.reports):
                meta=json.loads(report.read_text(encoding='utf-8'))
                key=report.name.split('.')[0].upper()
                if not key.isidentifier() or not key.isascii():raise ValueError('Invalid asset identifier')
                lines.append('#define ASSET_'+key+'_BYTES '+str(meta['asset']['total_bytes'])+'u')
            text='\n'.join(lines)+'\n'
            a.header.parent.mkdir(parents=True,exist_ok=True)
            if not a.header.exists() or a.header.read_text()!=text:a.header.write_text(text)
        print('Prepared art: hashes and binary structure verified')
    except (OSError,ValueError,KeyError,TypeError) as e:p.error(str(e))

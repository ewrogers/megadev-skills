#!/usr/bin/env python3
"""Inspect a data-only Megadev ISO; this does not validate mixed-mode audio tracks."""
import argparse
import json
from pathlib import Path
import sys
import zlib
from disc import inspect_iso
SIGNATURES={'US':(1412,0xF361AB57),'JP':(340,0x4571F623),'EU':(1390,0x6FFB4732)}
def inspect(path,region=None,expected=()):
    data=Path(path).read_bytes();result=inspect_iso(data)
    if region:
        length,checksum=SIGNATURES[region]
        if zlib.crc32(data[0x200:0x200+length])!=checksum:raise ValueError('Regional boot block does not match '+region)
    missing=set(expected)-set(result['files'])
    if missing:raise ValueError('Missing disc files: '+', '.join(sorted(missing)))
    return result
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('image',type=Path)
    parser.add_argument('--region',choices=SIGNATURES);parser.add_argument('--expect',action='append',default=[])
    args=parser.parse_args()
    try:print(json.dumps(inspect(args.image,args.region,args.expect),indent=2))
    except (ValueError,OSError) as error:print(error,file=sys.stderr);raise SystemExit(2)
